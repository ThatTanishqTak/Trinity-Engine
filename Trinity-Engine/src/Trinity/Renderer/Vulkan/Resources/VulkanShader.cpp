#include "Trinity/Renderer/Vulkan/Resources/VulkanShader.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

#include <spirv_cross/spirv_cross.hpp>

#include <fstream>

namespace Trinity
{
    namespace
    {
        ReflectedVertexFormat VertexFormatFrom(const spirv_cross::SPIRType& type)
        {
            const uint32_t l_Vector = type.vecsize;

            if (type.basetype == spirv_cross::SPIRType::Float)
            {
                switch (l_Vector)
                {
                    case 1:
                        return ReflectedVertexFormat::Float;
                    case 2:
                        return ReflectedVertexFormat::Float2;
                    case 3:
                        return ReflectedVertexFormat::Float3;
                    case 4:
                        return ReflectedVertexFormat::Float4;
                    default:
                        return ReflectedVertexFormat::Unknown;
                }
            }

            if (type.basetype == spirv_cross::SPIRType::Int)
            {
                switch (l_Vector)
                {
                    case 1:
                        return ReflectedVertexFormat::Int;
                    case 2:
                        return ReflectedVertexFormat::Int2;
                    case 3:
                        return ReflectedVertexFormat::Int3;
                    case 4:
                        return ReflectedVertexFormat::Int4;
                    default:
                        return ReflectedVertexFormat::Unknown;
                }
            }

            if (type.basetype == spirv_cross::SPIRType::UInt)
            {
                switch (l_Vector)
                {
                    case 1:
                        return ReflectedVertexFormat::UInt;
                    case 2:
                        return ReflectedVertexFormat::UInt2;
                    case 3:
                        return ReflectedVertexFormat::UInt3;
                    case 4:
                        return ReflectedVertexFormat::UInt4;
                    default:
                        return ReflectedVertexFormat::Unknown;
                }
            }

            return ReflectedVertexFormat::Unknown;
        }

        uint32_t SpecConstantSize(const spirv_cross::SPIRType& type)
        {
            uint32_t l_Bytes = 0;
            switch (type.basetype)
            {
                case spirv_cross::SPIRType::Boolean:
                case spirv_cross::SPIRType::Int:
                case spirv_cross::SPIRType::UInt:
                case spirv_cross::SPIRType::Float:
                    l_Bytes = 4;
                    break;
                case spirv_cross::SPIRType::Int64:
                case spirv_cross::SPIRType::UInt64:
                case spirv_cross::SPIRType::Double:
                    l_Bytes = 8;
                    break;
                case spirv_cross::SPIRType::Short:
                case spirv_cross::SPIRType::UShort:
                case spirv_cross::SPIRType::Half:
                    l_Bytes = 2;
                    break;
                case spirv_cross::SPIRType::SByte:
                case spirv_cross::SPIRType::UByte:
                    l_Bytes = 1;
                    break;
                default:
                    l_Bytes = 0;
                    break;
            }

            return l_Bytes * type.vecsize * type.columns;
        }

        ReflectedDescriptorBinding* FindOrAppendBinding(std::vector<ReflectedDescriptorBinding>& bindings, uint32_t set, uint32_t binding)
        {
            for (auto& it_Existing : bindings)
            {
                if (it_Existing.Set == set && it_Existing.Binding == binding)
                {
                    return &it_Existing;
                }
            }

            bindings.emplace_back();
            ReflectedDescriptorBinding& a_New = bindings.back();
            a_New.Set = set;
            a_New.Binding = binding;

            return &a_New;
        }

        ReflectedPushConstantRange* FindOrAppendPushConstant(std::vector<ReflectedPushConstantRange>& ranges, uint32_t offset, uint32_t size)
        {
            for (auto& it_Existing : ranges)
            {
                if (it_Existing.Offset == offset && it_Existing.Size == size)
                {
                    return &it_Existing;
                }
            }

            ranges.emplace_back();
            ReflectedPushConstantRange& a_New = ranges.back();
            a_New.Offset = offset;
            a_New.Size = size;

            return &a_New;
        }
    }

    VulkanShader::VulkanShader(VkDevice device, const ShaderSpecification& specification) : m_Device(device)
    {
        m_Specification = specification;

        m_EntryPointStorage.reserve(specification.Modules.size());

        for (const auto& it_Module : specification.Modules)
        {
            std::vector<uint32_t> l_Code = it_Module.SpvBlob.empty() ? ReadSpvFile(it_Module.SpvPath) : it_Module.SpvBlob;

            if (l_Code.empty())
            {
                TR_CORE_ERROR("Shader module is empty: {}", it_Module.SpvPath);

                continue;
            }

            VkShaderModuleCreateInfo l_CreateInfo{};
            l_CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            l_CreateInfo.codeSize = l_Code.size() * sizeof(uint32_t);
            l_CreateInfo.pCode = l_Code.data();

            VkShaderModule l_ShaderModule = VK_NULL_HANDLE;
            VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device, &l_CreateInfo, nullptr, &l_ShaderModule), "Failed vkCreateShaderModule");
            m_Modules.push_back(l_ShaderModule);

            m_EntryPointStorage.push_back(it_Module.EntryPoint.empty() ? std::string("main") : it_Module.EntryPoint);

            VkPipelineShaderStageCreateInfo l_StageInfo{};
            l_StageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            l_StageInfo.stage = VulkanUtilities::ToVkShaderStage(it_Module.Stage);
            l_StageInfo.module = l_ShaderModule;
            l_StageInfo.pName = m_EntryPointStorage.back().c_str();
            m_StageCreateInfos.push_back(l_StageInfo);

            ReflectedEntryPoint l_EntryPoint{};
            l_EntryPoint.Stage = it_Module.Stage;
            l_EntryPoint.Name = m_EntryPointStorage.back();
            m_Reflection.EntryPoints.push_back(l_EntryPoint);

            Reflect(l_Code, it_Module.Stage, m_EntryPointStorage.back());
        }
    }

    VulkanShader::~VulkanShader()
    {
        ReleaseModules();
    }

    void VulkanShader::ReleaseModules()
    {
        for (auto it_Module : m_Modules)
        {
            if (it_Module != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(m_Device, it_Module, nullptr);
            }
        }

        m_Modules.clear();
        m_StageCreateInfos.clear();
        m_EntryPointStorage.clear();
    }

    std::vector<uint32_t> VulkanShader::ReadSpvFile(const std::string& path) const
    {
#ifdef TRINITY_SHADER_DIR
        std::string l_FullPath = std::string(TRINITY_SHADER_DIR) + path;
#else
        std::string l_FullPath = path;
#endif
        std::ifstream l_File(l_FullPath, std::ios::ate | std::ios::binary);

        if (!l_File.is_open())
        {
            return {};
        }

        size_t l_FileSize = static_cast<size_t>(l_File.tellg());
        std::vector<uint32_t> l_Buffer(l_FileSize / sizeof(uint32_t));

        l_File.seekg(0);
        l_File.read(reinterpret_cast<char*>(l_Buffer.data()), static_cast<std::streamsize>(l_FileSize));

        return l_Buffer;
    }

    void VulkanShader::Reflect(const std::vector<uint32_t>& spirv, ShaderStage stage, const std::string& entryPoint)
    {
        try
        {
            spirv_cross::Compiler l_Compiler(spirv);
            spirv_cross::ShaderResources l_Resources = l_Compiler.get_shader_resources();
            ShaderStageFlags l_StageFlag = ToShaderStageFlags(stage);

            auto a_AddResource = [&](const spirv_cross::Resource& resource, ReflectedResourceType type)
            {
                const uint32_t l_Set = l_Compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                const uint32_t l_Binding = l_Compiler.get_decoration(resource.id, spv::DecorationBinding);
                const spirv_cross::SPIRType& l_Type = l_Compiler.get_type(resource.type_id);

                ReflectedDescriptorBinding* a_Entry = FindOrAppendBinding(m_Reflection.DescriptorBindings, l_Set, l_Binding);
                a_Entry->Type = type;
                a_Entry->Stages |= l_StageFlag;
                if (a_Entry->Name.empty())
                {
                    a_Entry->Name = resource.name;
                }

                if (l_Type.array.empty())
                {
                    a_Entry->Count = 1;
                    a_Entry->IsRuntimeArray = false;
                }
                else
                {
                    const uint32_t l_Dim = l_Type.array.front();
                    if (l_Dim == 0)
                    {
                        a_Entry->Count = 0;
                        a_Entry->IsRuntimeArray = true;
                    }
                    else
                    {
                        a_Entry->Count = l_Dim;
                        a_Entry->IsRuntimeArray = false;
                    }
                }
            };

            for (const auto& it_Resource : l_Resources.uniform_buffers)
            {
                a_AddResource(it_Resource, ReflectedResourceType::UniformBuffer);
            }

            for (const auto& it_Resource : l_Resources.storage_buffers)
            {
                a_AddResource(it_Resource, ReflectedResourceType::StorageBuffer);
            }

            for (const auto& it_Resource : l_Resources.sampled_images)
            {
                a_AddResource(it_Resource, ReflectedResourceType::CombinedImageSampler);
            }

            for (const auto& it_Resource : l_Resources.separate_images)
            {
                a_AddResource(it_Resource, ReflectedResourceType::SampledImage);
            }

            for (const auto& it_Resource : l_Resources.separate_samplers)
            {
                a_AddResource(it_Resource, ReflectedResourceType::Sampler);
            }

            for (const auto& it_Resource : l_Resources.storage_images)
            {
                a_AddResource(it_Resource, ReflectedResourceType::StorageImage);
            }

            for (const auto& it_Resource : l_Resources.subpass_inputs)
            {
                a_AddResource(it_Resource, ReflectedResourceType::InputAttachment);
            }

            for (const auto& it_Resource : l_Resources.acceleration_structures)
            {
                a_AddResource(it_Resource, ReflectedResourceType::AccelerationStructure);
            }

            for (const auto& it_Resource : l_Resources.push_constant_buffers)
            {
                spirv_cross::SmallVector<spirv_cross::BufferRange> l_Ranges = l_Compiler.get_active_buffer_ranges(it_Resource.id);
                if (l_Ranges.empty())
                {
                    continue;
                }

                uint32_t l_MinOffset = l_Ranges[0].offset;
                uint32_t l_MaxEnd = l_Ranges[0].offset + static_cast<uint32_t>(l_Ranges[0].range);

                for (size_t i = 1; i < l_Ranges.size(); i++)
                {
                    const uint32_t l_End = l_Ranges[i].offset + static_cast<uint32_t>(l_Ranges[i].range);
                    if (l_Ranges[i].offset < l_MinOffset)
                    {
                        l_MinOffset = l_Ranges[i].offset;
                    }
                    if (l_End > l_MaxEnd)
                    {
                        l_MaxEnd = l_End;
                    }
                }

                ReflectedPushConstantRange* a_Entry = FindOrAppendPushConstant(m_Reflection.PushConstants, l_MinOffset, l_MaxEnd - l_MinOffset);
                a_Entry->Stages |= l_StageFlag;
                if (a_Entry->Name.empty())
                {
                    a_Entry->Name = it_Resource.name;
                }
            }

            if (stage == ShaderStage::Vertex)
            {
                for (const auto& it_Input : l_Resources.stage_inputs)
                {
                    const uint32_t l_Location = l_Compiler.get_decoration(it_Input.id, spv::DecorationLocation);
                    const spirv_cross::SPIRType& l_Type = l_Compiler.get_type(it_Input.type_id);

                    ReflectedVertexInput l_Entry{};
                    l_Entry.Location = l_Location;
                    l_Entry.Format = VertexFormatFrom(l_Type);
                    l_Entry.Name = it_Input.name;
                    m_Reflection.VertexInputs.push_back(l_Entry);
                }
            }

            spirv_cross::SmallVector<spirv_cross::SpecializationConstant> l_SpecConstants = l_Compiler.get_specialization_constants();
            for (const auto& it_Constant : l_SpecConstants)
            {
                const spirv_cross::SPIRConstant& l_ConstantValue = l_Compiler.get_constant(it_Constant.id);
                const spirv_cross::SPIRType& l_Type = l_Compiler.get_type(l_ConstantValue.constant_type);

                ReflectedSpecializationConstant l_Entry{};
                l_Entry.ConstantID = it_Constant.constant_id;
                l_Entry.Size = SpecConstantSize(l_Type);
                l_Entry.Stages = l_StageFlag;
                l_Entry.Name = l_Compiler.get_name(it_Constant.id);
                m_Reflection.SpecializationConstants.push_back(l_Entry);
            }

            (void)entryPoint;
        }
        catch (const std::exception& a_Exception)
        {
            TR_CORE_ERROR("SPIRV-Cross reflection failed: {}", a_Exception.what());
        }
    }
}