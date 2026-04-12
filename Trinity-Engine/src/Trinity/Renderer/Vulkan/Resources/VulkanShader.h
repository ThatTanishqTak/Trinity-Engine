#pragma once

#include "Trinity/Renderer/Resources/Shader.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace Trinity
{
    class VulkanShader final : public Shader
    {
    public:
        VulkanShader(VkDevice device, const ShaderSpecification& specification);
        ~VulkanShader() override;

        const std::vector<VkPipelineShaderStageCreateInfo>& GetStageCreateInfos() const { return m_StageCreateInfos; }

    private:
        std::vector<uint32_t> ReadSpvFile(const std::string& path) const;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        std::vector<VkShaderModule> m_Modules;
        std::vector<VkPipelineShaderStageCreateInfo> m_StageCreateInfos;
    };
}