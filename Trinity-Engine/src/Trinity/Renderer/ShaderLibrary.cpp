#include "Trinity/Renderer/ShaderLibrary.h"

#include "Trinity/Utilities/FileManagement.h"
#include "Trinity/Utilities/Log.h"

#ifdef TR_ENABLE_SHADER_COMPILATION
#include <shaderc/shaderc.hpp>
#endif

#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace Trinity
{
	static constexpr uint32_t s_SpvMagic = 0x07230203;
	static constexpr uint32_t s_OpTypeInt = 21;
	static constexpr uint32_t s_OpTypeFloat = 22;
	static constexpr uint32_t s_OpTypeVector = 23;
	static constexpr uint32_t s_OpTypeMatrix = 24;
	static constexpr uint32_t s_OpTypeStruct = 30;
	static constexpr uint32_t s_OpTypePointer = 32;
	static constexpr uint32_t s_OpDecorate = 47;
	static constexpr uint32_t s_OpMemberDecorate = 48;
	static constexpr uint32_t s_OpVariable = 59;
	static constexpr uint32_t s_DecorationBinding = 33;
	static constexpr uint32_t s_DecorationDescriptorSet = 34;
	static constexpr uint32_t s_DecorationBlock = 2;
	static constexpr uint32_t s_DecorationOffset = 35;
	static constexpr uint32_t s_StorageClassUniform = 2;
	static constexpr uint32_t s_StorageClassUniformConstant = 0;
	static constexpr uint32_t s_StorageClassPushConstant = 9;
	static constexpr uint32_t s_StorageClassStorageBuffer = 12;

	const Shader* ShaderLibrary::Load(const std::string& name, const std::string& spvPath, ShaderStage stage)
	{
		const std::vector<char> l_Bytes = Utilities::FileManagement::LoadFromFile(spvPath);

		if (l_Bytes.empty() || (l_Bytes.size() % 4) != 0)
		{
			TR_CORE_CRITICAL("ShaderLibrary::Load — invalid SPIR-V file: {}", spvPath.c_str());
			std::abort();
		}

		Shader l_Shader{};
		l_Shader.m_Name = name;
		l_Shader.m_SourcePath = spvPath;
		l_Shader.m_Stage = stage;
		l_Shader.m_SpirV.resize(l_Bytes.size() / 4);
		l_Shader.m_CompiledFromSource = false;

		std::memcpy(l_Shader.m_SpirV.data(), l_Bytes.data(), l_Bytes.size());

		if (l_Shader.m_SpirV[0] != s_SpvMagic)
		{
			TR_CORE_CRITICAL("ShaderLibrary::Load — file does not begin with SPIR-V magic: {}", spvPath.c_str());
			std::abort();
		}

		std::error_code l_Ec;
		l_Shader.m_LastModified = std::filesystem::last_write_time(spvPath, l_Ec);

		l_Shader.m_Reflection = ReflectSpirV(l_Shader.m_SpirV);

		m_Shaders[name] = std::move(l_Shader);

		TR_CORE_TRACE("ShaderLibrary: loaded '{}' from {}", name.c_str(), spvPath.c_str());

		return &m_Shaders.at(name);
	}

	const Shader* ShaderLibrary::Compile(const std::string& name, const std::string& sourcePath, ShaderStage stage)
	{
#ifndef TR_ENABLE_SHADER_COMPILATION
		TR_CORE_CRITICAL("ShaderLibrary::Compile — TR_ENABLE_SHADER_COMPILATION is not defined. Cannot compile '{}'", sourcePath.c_str());
		std::abort();
#else
		const std::vector<uint32_t> l_SpirV = CompileGlsl(sourcePath, stage);

		Shader l_Shader{};
		l_Shader.m_Name = name;
		l_Shader.m_SourcePath = sourcePath;
		l_Shader.m_Stage = stage;
		l_Shader.m_SpirV = l_SpirV;
		l_Shader.m_CompiledFromSource = true;

		std::error_code l_Ec;
		l_Shader.m_LastModified = std::filesystem::last_write_time(sourcePath, l_Ec);

		l_Shader.m_Reflection = ReflectSpirV(l_Shader.m_SpirV);

		m_Shaders[name] = std::move(l_Shader);

		TR_CORE_TRACE("ShaderLibrary: compiled '{}' from {}", name.c_str(), sourcePath.c_str());

		return &m_Shaders.at(name);
#endif
	}

	const Shader* ShaderLibrary::Get(const std::string& name) const
	{
		const auto a_It = m_Shaders.find(name);
		if (a_It == m_Shaders.end())
		{
			return nullptr;
		}

		return &a_It->second;
	}

	bool ShaderLibrary::Has(const std::string& name) const
	{
		return m_Shaders.count(name) > 0;
	}

	void ShaderLibrary::Remove(const std::string& name)
	{
		m_Shaders.erase(name);
	}

	void ShaderLibrary::Clear()
	{
		m_Shaders.clear();
	}

	bool ShaderLibrary::Reload(const std::string& name)
	{
		const auto a_It = m_Shaders.find(name);
		if (a_It == m_Shaders.end())
		{
			TR_CORE_WARN("ShaderLibrary::Reload — shader '{}' not found", name.c_str());

			return false;
		}

		const Shader& l_Existing = a_It->second;

		if (l_Existing.m_CompiledFromSource)
		{
			Compile(name, l_Existing.m_SourcePath, l_Existing.m_Stage);
		}
		else
		{
			Load(name, l_Existing.m_SourcePath, l_Existing.m_Stage);
		}

		return true;
	}

	void ShaderLibrary::HotReload()
	{
		for (auto& [l_Name, l_Shader] : m_Shaders)
		{
			std::error_code l_Ec;
			const auto l_CurrentModified = std::filesystem::last_write_time(l_Shader.m_SourcePath, l_Ec);

			if (l_Ec)
			{
				continue;
			}

			if (l_CurrentModified > l_Shader.m_LastModified)
			{
				TR_CORE_INFO("ShaderLibrary: hot-reloading '{}'", l_Name.c_str());
				Reload(l_Name);
			}
		}
	}

	const std::vector<uint32_t>* ShaderLibrary::GetSpirV(const std::string& name) const
	{
		const Shader* l_Shader = Get(name);
		if (!l_Shader)
		{
			return nullptr;
		}

		return &l_Shader->m_SpirV;
	}

	ShaderReflectionData ShaderLibrary::ReflectSpirV(const std::vector<uint32_t>& spirV)
	{
		ShaderReflectionData l_Result{};

		if (spirV.size() < 5 || spirV[0] != s_SpvMagic)
		{
			return l_Result;
		}

		struct IdDecoration
		{
			uint32_t m_Set = ~0u;
			uint32_t m_Binding = ~0u;
			bool m_HasBlock = false;
		};

		struct IdPointerInfo
		{
			uint32_t m_StorageClass = ~0u;
			uint32_t m_TypeId = 0;
		};

		struct MemberOffsetEntry
		{
			uint32_t m_StructId = 0;
			uint32_t m_Member = 0;
			uint32_t m_Offset = 0;
		};

		std::unordered_map<uint32_t, IdDecoration> l_Decorations;
		std::unordered_map<uint32_t, IdPointerInfo> l_Pointers;
		std::unordered_map<uint32_t, std::string> l_Names;
		std::vector<MemberOffsetEntry> l_MemberOffsets;
		std::unordered_map<uint32_t, bool> l_BlockIds;
		std::unordered_map<uint32_t, uint32_t> l_TypeSizes;
		std::unordered_map<uint32_t, std::vector<uint32_t>> l_StructMemberTypes;

		uint32_t l_Idx = 5;
		while (l_Idx < spirV.size())
		{
			const uint32_t l_Word = spirV[l_Idx];
			const uint32_t l_WordCount = l_Word >> 16;
			const uint32_t l_Opcode = l_Word & 0xFFFF;

			if (l_WordCount == 0 || l_Idx + l_WordCount > spirV.size())
			{
				break;
			}

			if (l_Opcode == 5 && l_WordCount >= 3)
			{
				const uint32_t l_Id = spirV[l_Idx + 1];
				const char* l_NamePtr = reinterpret_cast<const char*>(&spirV[l_Idx + 2]);
				const size_t l_MaxLen = (l_WordCount - 2) * 4;
				l_Names[l_Id] = std::string(l_NamePtr, strnlen(l_NamePtr, l_MaxLen));
			}
			else if (l_Opcode == s_OpTypeInt && l_WordCount >= 3)
			{
				const uint32_t l_ResultId = spirV[l_Idx + 1];
				const uint32_t l_Width = spirV[l_Idx + 2];
				l_TypeSizes[l_ResultId] = l_Width / 8;
			}
			else if (l_Opcode == s_OpTypeFloat && l_WordCount >= 3)
			{
				const uint32_t l_ResultId = spirV[l_Idx + 1];
				const uint32_t l_Width = spirV[l_Idx + 2];
				l_TypeSizes[l_ResultId] = l_Width / 8;
			}
			else if (l_Opcode == s_OpTypeVector && l_WordCount >= 4)
			{
				const uint32_t l_ResultId = spirV[l_Idx + 1];
				const uint32_t l_ComponentId = spirV[l_Idx + 2];
				const uint32_t l_Count = spirV[l_Idx + 3];
				const auto a_It = l_TypeSizes.find(l_ComponentId);
				if (a_It != l_TypeSizes.end())
				{
					l_TypeSizes[l_ResultId] = a_It->second * l_Count;
				}
			}
			else if (l_Opcode == s_OpTypeMatrix && l_WordCount >= 4)
			{
				const uint32_t l_ResultId = spirV[l_Idx + 1];
				const uint32_t l_ColumnId = spirV[l_Idx + 2];
				const uint32_t l_ColCount = spirV[l_Idx + 3];
				const auto a_It = l_TypeSizes.find(l_ColumnId);
				if (a_It != l_TypeSizes.end())
				{
					l_TypeSizes[l_ResultId] = a_It->second * l_ColCount;
				}
			}
			else if (l_Opcode == s_OpTypeStruct && l_WordCount >= 2)
			{
				const uint32_t l_ResultId = spirV[l_Idx + 1];
				std::vector<uint32_t> l_Members;
				for (uint32_t l_M = 2; l_M < l_WordCount; ++l_M)
				{
					l_Members.push_back(spirV[l_Idx + l_M]);
				}
				l_StructMemberTypes[l_ResultId] = std::move(l_Members);
			}
			else if (l_Opcode == s_OpDecorate && l_WordCount >= 3)
			{
				const uint32_t l_TargetId = spirV[l_Idx + 1];
				const uint32_t l_Decoration = spirV[l_Idx + 2];

				if (l_Decoration == s_DecorationDescriptorSet && l_WordCount >= 4)
				{
					l_Decorations[l_TargetId].m_Set = spirV[l_Idx + 3];
				}
				else if (l_Decoration == s_DecorationBinding && l_WordCount >= 4)
				{
					l_Decorations[l_TargetId].m_Binding = spirV[l_Idx + 3];
				}
				else if (l_Decoration == s_DecorationBlock)
				{
					l_Decorations[l_TargetId].m_HasBlock = true;
					l_BlockIds[l_TargetId] = true;
				}
			}
			else if (l_Opcode == s_OpMemberDecorate && l_WordCount >= 4)
			{
				const uint32_t l_StructId = spirV[l_Idx + 1];
				const uint32_t l_Member = spirV[l_Idx + 2];
				const uint32_t l_Decoration = spirV[l_Idx + 3];

				if (l_Decoration == s_DecorationOffset && l_WordCount >= 5)
				{
					MemberOffsetEntry l_Entry{};
					l_Entry.m_StructId = l_StructId;
					l_Entry.m_Member = l_Member;
					l_Entry.m_Offset = spirV[l_Idx + 4];
					l_MemberOffsets.push_back(l_Entry);
				}
			}
			else if (l_Opcode == s_OpTypePointer && l_WordCount >= 4)
			{
				const uint32_t l_ResultId = spirV[l_Idx + 1];
				const uint32_t l_StorageClass = spirV[l_Idx + 2];
				const uint32_t l_TypeId = spirV[l_Idx + 3];

				l_Pointers[l_ResultId] = { l_StorageClass, l_TypeId };
			}
			else if (l_Opcode == s_OpVariable && l_WordCount >= 4)
			{
				const uint32_t l_TypeId = spirV[l_Idx + 1];
				const uint32_t l_ResultId = spirV[l_Idx + 2];
				const uint32_t l_StorageClass = spirV[l_Idx + 3];

				const auto a_PtrIt = l_Pointers.find(l_TypeId);
				if (a_PtrIt == l_Pointers.end())
				{
					l_Idx += l_WordCount;
					continue;
				}

				if (l_StorageClass == s_StorageClassUniform || l_StorageClass == s_StorageClassUniformConstant || l_StorageClass == s_StorageClassStorageBuffer)
				{
					const auto& l_Dec = l_Decorations[l_ResultId];
					if (l_Dec.m_Set != ~0u && l_Dec.m_Binding != ~0u)
					{
						ShaderBindingInfo l_Binding{};
						l_Binding.m_Set = l_Dec.m_Set;
						l_Binding.m_Binding = l_Dec.m_Binding;
						l_Binding.m_Count = 1;

						const auto a_NameIt = l_Names.find(l_ResultId);
						if (a_NameIt != l_Names.end())
						{
							l_Binding.m_Name = a_NameIt->second;
						}

						l_Result.m_Bindings.push_back(l_Binding);
					}
				}
				else if (l_StorageClass == s_StorageClassPushConstant)
				{
					const uint32_t l_PointedTypeId = a_PtrIt->second.m_TypeId;

					uint32_t l_MaxEndByte = 0;
					uint32_t l_MaxMemberIdx = 0;

					for (const auto& it_Entry : l_MemberOffsets)
					{
						if (it_Entry.m_StructId == l_PointedTypeId && it_Entry.m_Offset > l_MaxEndByte)
						{
							l_MaxEndByte = it_Entry.m_Offset;
							l_MaxMemberIdx = it_Entry.m_Member;
						}
					}

					if (l_MaxEndByte > 0)
					{
						uint32_t l_LastMemberSize = 4;

						const auto a_StructIt = l_StructMemberTypes.find(l_PointedTypeId);
						if (a_StructIt != l_StructMemberTypes.end() && l_MaxMemberIdx < static_cast<uint32_t>(a_StructIt->second.size()))
						{
							const auto a_SizeIt = l_TypeSizes.find(a_StructIt->second[l_MaxMemberIdx]);
							if (a_SizeIt != l_TypeSizes.end())
							{
								l_LastMemberSize = a_SizeIt->second;
							}
						}

						l_Result.m_PushConstants.m_Offset = 0;
						l_Result.m_PushConstants.m_Size = l_MaxEndByte + l_LastMemberSize;
					}
				}
			}

			l_Idx += l_WordCount;
		}

		std::sort(l_Result.m_Bindings.begin(), l_Result.m_Bindings.end(), [](const ShaderBindingInfo& a, const ShaderBindingInfo& b)
			{
				if (a.m_Set != b.m_Set)
				{
					return a.m_Set < b.m_Set;
				}
				
				return a.m_Binding < b.m_Binding;
			});

		return l_Result;
	}

#ifdef TR_ENABLE_SHADER_COMPILATION
	class FileIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		explicit FileIncluder(std::filesystem::path baseDir) : m_BaseDir(std::move(baseDir))
		{

		}

		shaderc_include_result* GetInclude(const char* requestedSource, shaderc_include_type, const char* requestingSource, size_t) override
		{
			std::filesystem::path l_RequestingDir = std::filesystem::path(requestingSource).parent_path();
			std::filesystem::path l_Resolved = l_RequestingDir / requestedSource;

			if (!std::filesystem::exists(l_Resolved))
			{
				l_Resolved = m_BaseDir / requestedSource;
			}

			auto* l_Data = new IncludeData{};

			if (std::filesystem::exists(l_Resolved))
			{
				const std::vector<char> l_Bytes = Utilities::FileManagement::LoadFromFile(l_Resolved.string());
				l_Data->m_Content.assign(l_Bytes.begin(), l_Bytes.end());
				l_Data->m_SourceName = l_Resolved.string();
			}
			else
			{
				l_Data->m_Content = "Cannot find include: " + std::string(requestedSource);
				l_Data->m_SourceName = "";
			}

			l_Data->m_Result.source_name = l_Data->m_SourceName.c_str();
			l_Data->m_Result.source_name_length = l_Data->m_SourceName.size();
			l_Data->m_Result.content = l_Data->m_Content.c_str();
			l_Data->m_Result.content_length = l_Data->m_Content.size();
			l_Data->m_Result.user_data = l_Data;

			return &l_Data->m_Result;
		}

		void ReleaseInclude(shaderc_include_result* data) override
		{
			delete static_cast<IncludeData*>(data->user_data);
		}

	private:
		struct IncludeData
		{
			std::string m_SourceName;
			std::string m_Content;
			shaderc_include_result m_Result{};
		};

		std::filesystem::path m_BaseDir;
	};
#endif

	std::vector<uint32_t> ShaderLibrary::CompileGlsl(const std::string& sourcePath, ShaderStage stage)
	{
#ifndef TR_ENABLE_SHADER_COMPILATION
		(void)sourcePath;
		(void)stage;

		return {};
#else
		const std::vector<char> l_Source = Utilities::FileManagement::LoadFromFile(sourcePath);
		if (l_Source.empty())
		{
			TR_CORE_CRITICAL("ShaderLibrary::CompileGlsl — empty source file: {}", sourcePath.c_str());

			std::abort();
		}

		const std::string l_SourceStr(l_Source.begin(), l_Source.end());

		shaderc_shader_kind l_Kind = shaderc_vertex_shader;
		switch (stage)
		{
			case ShaderStage::Vertex:   l_Kind = shaderc_vertex_shader;   break;
			case ShaderStage::Fragment: l_Kind = shaderc_fragment_shader; break;
			case ShaderStage::Compute:  l_Kind = shaderc_compute_shader;  break;
			default: break;
		}

		shaderc::Compiler l_Compiler;
		shaderc::CompileOptions l_Options;
		l_Options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		l_Options.SetOptimizationLevel(shaderc_optimization_level_performance);

		const std::filesystem::path l_SourceDir = std::filesystem::path(sourcePath).parent_path();
		l_Options.SetIncluder(std::make_unique<FileIncluder>(l_SourceDir));

		const shaderc::SpvCompilationResult l_Result = l_Compiler.CompileGlslToSpv(l_SourceStr, l_Kind, sourcePath.c_str(), l_Options);

		if (l_Result.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			TR_CORE_CRITICAL("ShaderLibrary::CompileGlsl — compilation failed for '{}': {}", sourcePath.c_str(), l_Result.GetErrorMessage().c_str());

			std::abort();
		}

		return { l_Result.cbegin(), l_Result.cend() };
#endif
	}
}