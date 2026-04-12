#include "Trinity/Renderer/Vulkan/Resources/VulkanShader.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

#include <fstream>

namespace Trinity
{
    VulkanShader::VulkanShader(VkDevice device, const ShaderSpecification& specification) : m_Device(device)
    {
        m_Specification = specification;

        for (const auto& it_Module : specification.Modules)
        {
            std::vector<uint32_t> l_Code = ReadSpvFile(it_Module.SpvPath);

            VkShaderModuleCreateInfo l_CreateInfo{};
            l_CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            l_CreateInfo.codeSize = l_Code.size() * sizeof(uint32_t);
            l_CreateInfo.pCode = l_Code.data();

            VkShaderModule l_ShaderModule;
            VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device, &l_CreateInfo, nullptr, &l_ShaderModule), "Failed vkCreateShaderModule");
            m_Modules.push_back(l_ShaderModule);

            VkPipelineShaderStageCreateInfo l_StageInfo{};
            l_StageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            l_StageInfo.stage = VulkanUtilities::ToVkShaderStage(it_Module.Stage);
            l_StageInfo.module = l_ShaderModule;
            l_StageInfo.pName = "main";
            m_StageCreateInfos.push_back(l_StageInfo);
        }
    }

    VulkanShader::~VulkanShader()
    {
        for (auto it_Module : m_Modules)
        {
            vkDestroyShaderModule(m_Device, it_Module, nullptr);
        }
    }

    std::vector<uint32_t> VulkanShader::ReadSpvFile(const std::string& path) const
    {
        std::ifstream l_File(path, std::ios::ate | std::ios::binary);

        if (!l_File.is_open())
        {
            TR_CORE_ERROR("Failed to open shader file: {}", path);
            return {};
        }

        size_t l_FileSize = static_cast<size_t>(l_File.tellg());
        std::vector<uint32_t> l_Buffer(l_FileSize / sizeof(uint32_t));

        l_File.seekg(0);
        l_File.read(reinterpret_cast<char*>(l_Buffer.data()), static_cast<std::streamsize>(l_FileSize));

        return l_Buffer;
    }
}