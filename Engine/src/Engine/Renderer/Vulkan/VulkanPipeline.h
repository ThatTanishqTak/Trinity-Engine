#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace Engine
{
    class VulkanPipeline
    {
    public:
        void Initialize(VkDevice device);
        void Shutdown();

        void CreateGraphicsPipeline(VkRenderPass renderPass, const VkVertexInputBindingDescription& bindingDescription,
            const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, const char* vertexSpvPath, const char* fragmentSpvPath,
            bool depthEnabled, const std::vector<VkDescriptorSetLayout>& setLayouts = {}, const std::vector<VkPushConstantRange>& pushConstantRanges = {});

        void Cleanup();

        VkPipeline GetHandle() const { return m_Pipeline; }
        VkPipelineLayout GetLayout() const { return m_PipelineLayout; }

    private:
        VkShaderModule CreateShaderModule(const std::vector<char>& code) const;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;

        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
    };
}