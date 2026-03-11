#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

namespace Trinity
{
	enum class ShaderStage : uint8_t
	{
		Vertex = 0,
		Fragment,
		Compute,
		Count
	};

	struct ShaderBindingInfo
	{
		std::string m_Name;
		uint32_t m_Set = 0;
		uint32_t m_Binding = 0;
		uint32_t m_DescriptorType = 0;
		uint32_t m_Count = 1;
	};

	struct ShaderPushConstantInfo
	{
		uint32_t m_Offset = 0;
		uint32_t m_Size = 0;
	};

	struct ShaderReflectionData
	{
		std::vector<ShaderBindingInfo> m_Bindings;
		ShaderPushConstantInfo m_PushConstants;
	};

	struct Shader
	{
		std::string m_Name;
		std::string m_SourcePath;
		ShaderStage m_Stage = ShaderStage::Vertex;
		std::vector<uint32_t> m_SpirV;
		ShaderReflectionData m_Reflection;
		std::filesystem::file_time_type m_LastModified{};
		bool m_CompiledFromSource = false;
	};
}