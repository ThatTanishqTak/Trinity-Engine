#pragma once

#include "Trinity/Renderer/Resources/Pipeline.h"

#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanPipeline final : public Pipeline
    {
    public:
        VulkanPipeline(VkDevice device, const PipelineSpecification& specification);
        ~VulkanPipeline() override;

        VkPipeline GetPipeline() const { return m_Pipeline; }
        VkPipelineLayout GetLayout() const { return m_PipelineLayout; }
        VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    };
}