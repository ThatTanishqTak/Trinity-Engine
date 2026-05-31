#include <Trinity/Renderer/Shaders/ShaderCompiler.h>

#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>

#include <cstring>
#include <cctype>
#include <cstdlib>
#include <format>

#include <Trinity/Core/FileManagement.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    static ShaderResourceKind MapCategory(slang::ParameterCategory category)
    {
        switch (category)
        {
            case slang::ParameterCategory::PushConstantBuffer: return ShaderResourceKind::PushConstant;
            case slang::ParameterCategory::ConstantBuffer: return ShaderResourceKind::UniformBuffer;
            case slang::ParameterCategory::ShaderResource: return ShaderResourceKind::SampledTexture;
            case slang::ParameterCategory::UnorderedAccess: return ShaderResourceKind::StorageResource;
            case slang::ParameterCategory::SamplerState: return ShaderResourceKind::Sampler;
            default: return ShaderResourceKind::Unknown;
        }
    }

    static SlangCompileTarget ToSlangTarget(ShaderTargetFormat target)
    {
        switch (target)
        {
            case ShaderTargetFormat::Metal: return SLANG_METAL;
            case ShaderTargetFormat::Dxil: return SLANG_DXIL;
            default: return SLANG_SPIRV;
        }
    }

    static const char* TargetProfile(ShaderTargetFormat target)
    {
        switch (target)
        {
            case ShaderTargetFormat::Metal: return "metallib";
            case ShaderTargetFormat::Dxil: return "sm_6_5";
            default: return "spirv_1_5";
        }
    }

    static constexpr const char* k_CacheVersionTag = "TSC1;spirv_1_5;emit_direct;col_major;use_ep_name";

    static uint64_t HashFnv1a(const std::string& data)
    {
        uint64_t l_Hash = 1469598103934665603ull;
        for (char l_Char : data)
        {
            l_Hash ^= static_cast<uint8_t>(l_Char);
            l_Hash *= 1099511628211ull;
        }

        return l_Hash;
    }

    static void AppendU32(std::vector<uint8_t>& buffer, uint32_t value)
    {
        buffer.push_back(static_cast<uint8_t>(value & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    }

    static void AppendString(std::vector<uint8_t>& buffer, const std::string& value)
    {
        AppendU32(buffer, static_cast<uint32_t>(value.size()));
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

    static ShaderDiagnosticSeverity ParseSeverity(const std::string& level)
    {
        if (level == "error" || level == "fatal")
        {
            return ShaderDiagnosticSeverity::Error;
        }

        if (level == "warning")
        {
            return ShaderDiagnosticSeverity::Warning;
        }

        return ShaderDiagnosticSeverity::Info;
    }

    static std::vector<ShaderDiagnostic> ParseDiagnostics(const std::string& text)
    {
        std::vector<ShaderDiagnostic> l_Messages;
        size_t l_Start = 0;

        while (l_Start < text.size())
        {
            size_t l_End = text.find('\n', l_Start);
            if (l_End == std::string::npos)
            {
                l_End = text.size();
            }

            std::string l_Line = text.substr(l_Start, l_End - l_Start);
            l_Start = l_End + 1;

            if (!l_Line.empty() && l_Line.back() == '\r')
            {
                l_Line.pop_back();
            }

            size_t l_CloseColon = l_Line.find("):");
            if (l_CloseColon == std::string::npos)
            {
                continue;
            }

            size_t l_Open = l_Line.rfind('(', l_CloseColon);
            if (l_Open == std::string::npos)
            {
                continue;
            }

            std::string l_Location = l_Line.substr(l_Open + 1, l_CloseColon - l_Open - 1);
            if (l_Location.empty() || !std::isdigit(static_cast<unsigned char>(l_Location[0])))
            {
                continue;
            }

            std::string l_Rest = l_Line.substr(l_CloseColon + 2);
            size_t l_FirstNonSpace = l_Rest.find_first_not_of(' ');
            if (l_FirstNonSpace == std::string::npos)
            {
                continue;
            }
            l_Rest = l_Rest.substr(l_FirstNonSpace);

            size_t l_SpaceAfterSeverity = l_Rest.find(' ');
            std::string l_SeverityWord = l_Rest.substr(0, l_SpaceAfterSeverity == std::string::npos ? l_Rest.size() : l_SpaceAfterSeverity);
            if (!l_SeverityWord.empty() && l_SeverityWord.back() == ':')
            {
                l_SeverityWord.pop_back();
            }

            ShaderDiagnostic l_Diagnostic;
            l_Diagnostic.Severity = ParseSeverity(l_SeverityWord);
            l_Diagnostic.File = l_Line.substr(0, l_Open);

            size_t l_Comma = l_Location.find(',');
            if (l_Comma == std::string::npos)
            {
                l_Diagnostic.Line = static_cast<uint32_t>(std::strtoul(l_Location.c_str(), nullptr, 10));
            }
            else
            {
                l_Diagnostic.Line = static_cast<uint32_t>(std::strtoul(l_Location.substr(0, l_Comma).c_str(), nullptr, 10));
                l_Diagnostic.Column = static_cast<uint32_t>(std::strtoul(l_Location.substr(l_Comma + 1).c_str(), nullptr, 10));
            }

            std::string l_AfterSeverity = l_SpaceAfterSeverity == std::string::npos ? std::string() : l_Rest.substr(l_SpaceAfterSeverity + 1);
            size_t l_CodeColon = l_AfterSeverity.find(':');
            if (l_CodeColon != std::string::npos)
            {
                l_Diagnostic.Code = l_AfterSeverity.substr(0, l_CodeColon);
                size_t l_MessageStart = l_AfterSeverity.find_first_not_of(' ', l_CodeColon + 1);
                l_Diagnostic.Message = l_MessageStart == std::string::npos ? std::string() : l_AfterSeverity.substr(l_MessageStart);
            }
            else
            {
                l_Diagnostic.Message = l_AfterSeverity;
            }

            l_Messages.push_back(std::move(l_Diagnostic));
        }

        return l_Messages;
    }

    struct ByteReader
    {
        const uint8_t* Data = nullptr;
        size_t Size = 0;
        size_t Cursor = 0;

        bool ReadU32(uint32_t& out)
        {
            if (Cursor + 4 > Size)
            {
                return false;
            }

            out = static_cast<uint32_t>(Data[Cursor]) | (static_cast<uint32_t>(Data[Cursor + 1]) << 8) | (static_cast<uint32_t>(Data[Cursor + 2]) << 16) | (static_cast<uint32_t>(Data[Cursor + 3]) << 24);
            Cursor += 4;

            return true;
        }

        bool ReadBytes(void* out, size_t count)
        {
            if (Cursor + count > Size)
            {
                return false;
            }

            std::memcpy(out, Data + Cursor, count);
            Cursor += count;

            return true;
        }

        bool ReadString(std::string& out)
        {
            uint32_t l_Length = 0;
            if (!ReadU32(l_Length) || Cursor + l_Length > Size)
            {
                return false;
            }

            out.assign(reinterpret_cast<const char*>(Data + Cursor), l_Length);
            Cursor += l_Length;

            return true;
        }
    };

    static std::vector<uint8_t> SerializeResult(const ShaderCompileResult& result)
    {
        std::vector<uint8_t> l_Buffer;
        const uint8_t l_Magic[4] = { 'T', 'S', 'C', '1' };
        l_Buffer.insert(l_Buffer.end(), l_Magic, l_Magic + 4);

        AppendU32(l_Buffer, static_cast<uint32_t>(result.SPIRV.size()));
        l_Buffer.insert(l_Buffer.end(), result.SPIRV.begin(), result.SPIRV.end());

        AppendU32(l_Buffer, result.Reflection.PushConstantSize);

        AppendU32(l_Buffer, static_cast<uint32_t>(result.Reflection.Bindings.size()));
        for (const ShaderBinding& l_Binding : result.Reflection.Bindings)
        {
            AppendU32(l_Buffer, static_cast<uint32_t>(l_Binding.Kind));
            AppendU32(l_Buffer, l_Binding.Set);
            AppendU32(l_Buffer, l_Binding.Binding);
            AppendU32(l_Buffer, l_Binding.Size);
            AppendString(l_Buffer, l_Binding.Name);
        }

        AppendU32(l_Buffer, static_cast<uint32_t>(result.Reflection.VertexInputs.size()));
        for (const ShaderVertexInput& l_Input : result.Reflection.VertexInputs)
        {
            AppendU32(l_Buffer, l_Input.Location);
            AppendString(l_Buffer, l_Input.Name);
        }

        return l_Buffer;
    }

    static bool DeserializeResult(const std::vector<uint8_t>& blob, ShaderCompileResult& result)
    {
        if (blob.size() < 4 || blob[0] != 'T' || blob[1] != 'S' || blob[2] != 'C' || blob[3] != '1')
        {
            return false;
        }

        ByteReader l_Reader{ blob.data(), blob.size(), 4 };

        uint32_t l_SpirVSize = 0;
        if (!l_Reader.ReadU32(l_SpirVSize))
        {
            return false;
        }

        result.SPIRV.resize(l_SpirVSize);
        if (l_SpirVSize > 0 && !l_Reader.ReadBytes(result.SPIRV.data(), l_SpirVSize))
        {
            return false;
        }

        if (!l_Reader.ReadU32(result.Reflection.PushConstantSize))
        {
            return false;
        }

        uint32_t l_BindingCount = 0;
        if (!l_Reader.ReadU32(l_BindingCount))
        {
            return false;
        }

        result.Reflection.Bindings.clear();
        for (uint32_t l_Index = 0; l_Index < l_BindingCount; ++l_Index)
        {
            ShaderBinding l_Binding;
            uint32_t l_Kind = 0;
            if (!l_Reader.ReadU32(l_Kind) || !l_Reader.ReadU32(l_Binding.Set) || !l_Reader.ReadU32(l_Binding.Binding) || !l_Reader.ReadU32(l_Binding.Size) || !l_Reader.ReadString(l_Binding.Name))
            {
                return false;
            }

            l_Binding.Kind = static_cast<ShaderResourceKind>(l_Kind);
            result.Reflection.Bindings.push_back(std::move(l_Binding));
        }

        uint32_t l_InputCount = 0;
        if (!l_Reader.ReadU32(l_InputCount))
        {
            return false;
        }

        result.Reflection.VertexInputs.clear();
        for (uint32_t l_Index = 0; l_Index < l_InputCount; ++l_Index)
        {
            ShaderVertexInput l_Input;
            if (!l_Reader.ReadU32(l_Input.Location) || !l_Reader.ReadString(l_Input.Name))
            {
                return false;
            }

            result.Reflection.VertexInputs.push_back(std::move(l_Input));
        }

        return true;
    }

    static ShaderReflection ExtractReflection(slang::ProgramLayout* layout)
    {
        ShaderReflection l_Reflection;
        if (layout == nullptr)
        {
            return l_Reflection;
        }

        unsigned l_ParameterCount = layout->getParameterCount();
        for (unsigned l_Index = 0; l_Index < l_ParameterCount; ++l_Index)
        {
            slang::VariableLayoutReflection* l_Parameter = layout->getParameterByIndex(l_Index);
            slang::ParameterCategory l_Category = l_Parameter->getCategory();

            ShaderBinding l_Binding;
            l_Binding.Name = l_Parameter->getName() ? l_Parameter->getName() : "";
            l_Binding.Kind = MapCategory(l_Category);
            l_Binding.Set = static_cast<uint32_t>(l_Parameter->getBindingSpace());
            l_Binding.Binding = static_cast<uint32_t>(l_Parameter->getBindingIndex());

            if (l_Category == slang::ParameterCategory::PushConstantBuffer)
            {
                slang::TypeLayoutReflection* l_TypeLayout = l_Parameter->getTypeLayout();
                if (l_TypeLayout != nullptr)
                {
                    slang::TypeLayoutReflection* l_Element = l_TypeLayout->getElementTypeLayout();
                    l_Binding.Size = static_cast<uint32_t>(l_Element ? l_Element->getSize() : l_TypeLayout->getSize());
                    l_Reflection.PushConstantSize = l_Binding.Size;
                }
            }

            l_Reflection.Bindings.push_back(l_Binding);
        }

        unsigned l_EntryPointCount = layout->getEntryPointCount();
        for (unsigned l_Index = 0; l_Index < l_EntryPointCount; ++l_Index)
        {
            slang::EntryPointReflection* l_EntryPoint = layout->getEntryPointByIndex(l_Index);
            if (l_EntryPoint == nullptr || l_EntryPoint->getStage() != SLANG_STAGE_VERTEX)
            {
                continue;
            }

            unsigned l_VaryingCount = l_EntryPoint->getParameterCount();
            for (unsigned l_Param = 0; l_Param < l_VaryingCount; ++l_Param)
            {
                slang::VariableLayoutReflection* l_Varying = l_EntryPoint->getParameterByIndex(l_Param);
                if (l_Varying->getCategory() != slang::ParameterCategory::VaryingInput)
                {
                    continue;
                }

                slang::TypeLayoutReflection* l_TypeLayout = l_Varying->getTypeLayout();
                if (l_TypeLayout != nullptr && l_TypeLayout->getKind() == slang::TypeReflection::Kind::Struct)
                {
                    unsigned l_FieldCount = l_TypeLayout->getFieldCount();
                    for (unsigned l_Field = 0; l_Field < l_FieldCount; ++l_Field)
                    {
                        slang::VariableLayoutReflection* l_FieldLayout = l_TypeLayout->getFieldByIndex(l_Field);

                        ShaderVertexInput l_Input;
                        l_Input.Name = l_FieldLayout->getName() ? l_FieldLayout->getName() : "";
                        l_Input.Location = static_cast<uint32_t>(l_FieldLayout->getBindingIndex());
                        l_Reflection.VertexInputs.push_back(l_Input);
                    }
                }
                else
                {
                    ShaderVertexInput l_Input;
                    l_Input.Name = l_Varying->getName() ? l_Varying->getName() : "";
                    l_Input.Location = static_cast<uint32_t>(l_Varying->getBindingIndex());
                    l_Reflection.VertexInputs.push_back(l_Input);
                }
            }
        }

        return l_Reflection;
    }

    struct ShaderCompiler::Implementation
    {
        Slang::ComPtr<slang::IGlobalSession> GlobalSession;
    };

    ShaderCompiler::ShaderCompiler() : m_Implementation(std::make_unique<Implementation>())
    {

    }

    ShaderCompiler::~ShaderCompiler()
    {
        Shutdown();
    }

    bool ShaderCompiler::Initialize()
    {
        if (m_Implementation->GlobalSession)
        {
            return true;
        }

        if (SLANG_FAILED(createGlobalSession(m_Implementation->GlobalSession.writeRef())))
        {
            TR_CORE_CRITICAL("ShaderCompiler: failed to create Slang global session");
            return false;
        }

        TR_CORE_INFO("ShaderCompiler: initialized");
        return true;
    }

    void ShaderCompiler::Shutdown()
    {
        m_Implementation->GlobalSession = nullptr;
    }

    void ShaderCompiler::SetCacheDirectory(const std::filesystem::path& a_Directory)
    {
        m_CacheDirectory = a_Directory;
        if (!m_CacheDirectory.empty())
        {
            FileManagement::EnsureDirectory(m_CacheDirectory);
        }
    }

    ShaderCompileResult ShaderCompiler::Compile(const std::filesystem::path& searchDirectory, const std::string& moduleName, const std::string& entryPoint, ShaderTargetFormat target)
    {
        ShaderCompileResult l_Result;

        if (!m_Implementation->GlobalSession)
        {
            l_Result.Diagnostics = "ShaderCompiler not initialized";
            return l_Result;
        }

        bool l_CacheEnabled = !m_CacheDirectory.empty();
        std::filesystem::path l_CacheFile;

        if (l_CacheEnabled)
        {
            std::filesystem::path l_SourcePath = searchDirectory / (moduleName + ".slang");
            std::optional<std::string> l_Source = FileManagement::ReadText(l_SourcePath);
            if (l_Source)
            {
                std::string l_KeyInput = *l_Source;
                l_KeyInput.push_back('\n');
                l_KeyInput.append(moduleName);
                l_KeyInput.push_back('\n');
                l_KeyInput.append(entryPoint);
                l_KeyInput.push_back('\n');
                l_KeyInput.append(std::to_string(static_cast<int>(target)));
                l_KeyInput.push_back('\n');
                l_KeyInput.append(k_CacheVersionTag);

                l_CacheFile = m_CacheDirectory / std::format("{:016x}.tsc", HashFnv1a(l_KeyInput));

                if (FileManagement::Exists(l_CacheFile))
                {
                    std::optional<std::vector<uint8_t>> l_Blob = FileManagement::ReadBinary(l_CacheFile);
                    if (l_Blob && DeserializeResult(*l_Blob, l_Result))
                    {
                        l_Result.Success = true;

                        TR_CORE_INFO("ShaderCompiler: cache hit {}:{}", moduleName, entryPoint);
                        return l_Result;
                    }
                }
            }
            else
            {
                l_CacheEnabled = false;
            }
        }

        auto a_Append = [&](slang::IBlob* blob)
        {
            if (blob != nullptr && blob->getBufferSize() > 0)
            {
                l_Result.Diagnostics.append(static_cast<const char*>(blob->getBufferPointer()), blob->getBufferSize());
            }
        };

        auto l_Fail = [&]() -> ShaderCompileResult
        {
            l_Result.Messages = ParseDiagnostics(l_Result.Diagnostics);
            return l_Result;
        };

        slang::TargetDesc l_TargetDescription{};
        l_TargetDescription.format = ToSlangTarget(target);
        l_TargetDescription.profile = m_Implementation->GlobalSession->findProfile(TargetProfile(target));

        std::vector<slang::CompilerOptionEntry> l_Options;

        slang::CompilerOptionEntry l_MatrixOption{};
        l_MatrixOption.name = slang::CompilerOptionName::MatrixLayoutColumn;
        l_MatrixOption.value.kind = slang::CompilerOptionValueKind::Int;
        l_MatrixOption.value.intValue0 = 1;
        l_Options.push_back(l_MatrixOption);

        if (target == ShaderTargetFormat::SPIRV)
        {
            slang::CompilerOptionEntry l_EmitOption{};
            l_EmitOption.name = slang::CompilerOptionName::EmitSpirvDirectly;
            l_EmitOption.value.kind = slang::CompilerOptionValueKind::Int;
            l_EmitOption.value.intValue0 = 1;
            l_Options.push_back(l_EmitOption);

            slang::CompilerOptionEntry l_EntryPointNameOption{};
            l_EntryPointNameOption.name = slang::CompilerOptionName::VulkanUseEntryPointName;
            l_EntryPointNameOption.value.kind = slang::CompilerOptionValueKind::Int;
            l_EntryPointNameOption.value.intValue0 = 1;
            l_Options.push_back(l_EntryPointNameOption);
        }

        std::string l_SearchPath = searchDirectory.string();
        const char* l_SearchPaths[] = { l_SearchPath.c_str() };

        slang::SessionDesc l_SessionDescription{};
        l_SessionDescription.targets = &l_TargetDescription;
        l_SessionDescription.targetCount = 1;
        l_SessionDescription.searchPaths = l_SearchPaths;
        l_SessionDescription.searchPathCount = 1;
        l_SessionDescription.compilerOptionEntries = l_Options.data();
        l_SessionDescription.compilerOptionEntryCount = static_cast<uint32_t>(l_Options.size());

        Slang::ComPtr<slang::ISession> l_Session;
        if (SLANG_FAILED(m_Implementation->GlobalSession->createSession(l_SessionDescription, l_Session.writeRef())))
        {
            l_Result.Diagnostics = "Failed to create Slang session";

            return l_Result;
        }

        Slang::ComPtr<slang::IBlob> l_Diagnostics;
        slang::IModule* l_Module = l_Session->loadModule(moduleName.c_str(), l_Diagnostics.writeRef());
        a_Append(l_Diagnostics);
        if (l_Module == nullptr)
        {
            return l_Result;
        }

        Slang::ComPtr<slang::IEntryPoint> l_EntryPoint;
        if (SLANG_FAILED(l_Module->findEntryPointByName(entryPoint.c_str(), l_EntryPoint.writeRef())) || l_EntryPoint == nullptr)
        {
            l_Result.Diagnostics.append("\nEntry point not found: ").append(entryPoint);

            return l_Result;
        }

        slang::IComponentType* l_Components[] = { l_Module, l_EntryPoint };
        Slang::ComPtr<slang::IComponentType> l_Program;
        l_Diagnostics = nullptr;
        if (SLANG_FAILED(l_Session->createCompositeComponentType(l_Components, 2, l_Program.writeRef(), l_Diagnostics.writeRef())))
        {
            a_Append(l_Diagnostics);

            return l_Result;
        }

        Slang::ComPtr<slang::IComponentType> l_Linked;
        l_Diagnostics = nullptr;
        if (SLANG_FAILED(l_Program->link(l_Linked.writeRef(), l_Diagnostics.writeRef())))
        {
            a_Append(l_Diagnostics);

            return l_Result;
        }

        Slang::ComPtr<slang::IBlob> l_SpirV;
        l_Diagnostics = nullptr;
        if (SLANG_FAILED(l_Linked->getEntryPointCode(0, 0, l_SpirV.writeRef(), l_Diagnostics.writeRef())))
        {
            a_Append(l_Diagnostics);

            return l_Result;
        }

        const uint8_t* l_Bytes = static_cast<const uint8_t*>(l_SpirV->getBufferPointer());
        l_Result.SPIRV.assign(l_Bytes, l_Bytes + l_SpirV->getBufferSize());
        l_Result.Reflection = ExtractReflection(l_Linked->getLayout());
        l_Result.Success = true;
        l_Result.Messages = ParseDiagnostics(l_Result.Diagnostics);

        if (l_CacheEnabled && !l_CacheFile.empty())
        {
            if (FileManagement::WriteBinary(l_CacheFile, SerializeResult(l_Result)))
            {
                TR_CORE_INFO("ShaderCompiler: cached {}:{}", moduleName, entryPoint);
            }
        }

        return l_Result;
    }
}