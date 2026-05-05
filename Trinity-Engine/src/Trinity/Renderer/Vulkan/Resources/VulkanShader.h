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
        const ShaderReflection& GetReflection() const override { return m_Reflection; }

        void ReleaseModules();
        bool HasModules() const { return !m_Modules.empty(); }

    private:
        std::vector<uint32_t> ReadSpvFile(const std::string& path) const;
        void Reflect(const std::vector<uint32_t>& spirv, ShaderStage stage, const std::string& entryPoint);

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        std::vector<VkShaderModule> m_Modules;
        std::vector<VkPipelineShaderStageCreateInfo> m_StageCreateInfos;
        std::vector<std::string> m_EntryPointStorage;
        ShaderReflection m_Reflection;
    };
}