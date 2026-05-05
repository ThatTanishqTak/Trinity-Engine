#include "Trinity/Renderer/Vulkan/Resources/VulkanShader.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

#include <fstream>

namespace Trinity
{
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
}