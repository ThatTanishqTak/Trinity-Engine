#pragma once

#include "Trinity/Renderer/Resources/ComputePipeline.h"

#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanComputePipeline final : public ComputePipeline
    {
    public:
        VulkanComputePipeline(VkDevice device, const ComputePipelineSpecification& specification, VkPipelineCache pipelineCache = VK_NULL_HANDLE);
        ~VulkanComputePipeline() override;

        VkPipeline GetPipeline() const { return m_Pipeline; }
        VkPipelineLayout GetLayout() const { return m_PipelineLayout; }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    };
}