#include "Trinity/Renderer/Vulkan/Resources/VulkanComputePipeline.h"

#include "Trinity/Renderer/Vulkan/Resources/VulkanShader.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

#include <vector>

namespace Trinity
{
    VulkanComputePipeline::VulkanComputePipeline(VkDevice device, const ComputePipelineSpecification& specification, VkPipelineCache pipelineCache) : m_Device(device)
    {
        m_Specification = specification;

        auto* a_VulkanShader = dynamic_cast<VulkanShader*>(specification.PipelineShader.get());
        if (a_VulkanShader == nullptr)
        {
            TR_CORE_ERROR("VulkanComputePipeline: shader is not a VulkanShader instance [{}]", specification.DebugName);

            return;
        }

        const auto& a_ShaderStages = a_VulkanShader->GetStageCreateInfos();
        if (a_ShaderStages.size() != 1)
        {
            TR_CORE_ERROR("VulkanComputePipeline [{}]: expected exactly 1 shader stage, got {}", specification.DebugName, a_ShaderStages.size());

            return;
        }

        if (a_ShaderStages[0].stage != VK_SHADER_STAGE_COMPUTE_BIT)
        {
            TR_CORE_ERROR("VulkanComputePipeline [{}]: shader stage is not VK_SHADER_STAGE_COMPUTE_BIT", specification.DebugName);

            return;
        }

        std::vector<VkDescriptorSetLayout> l_VkSetLayouts;
        l_VkSetLayouts.reserve(specification.DescriptorSetLayouts.size());
        for (const auto& it_Layout : specification.DescriptorSetLayouts)
        {
            if (!it_Layout)
            {
                TR_CORE_ERROR("VulkanComputePipeline [{}]: null entry in DescriptorSetLayouts; aborting layout creation", specification.DebugName);

                return;
            }

            VkDescriptorSetLayout l_Handle = reinterpret_cast<VkDescriptorSetLayout>(it_Layout->GetOpaqueHandle());
            if (l_Handle == VK_NULL_HANDLE)
            {
                TR_CORE_ERROR("VulkanComputePipeline [{}]: descriptor set layout has null Vulkan handle; aborting", specification.DebugName);

                return;
            }

            l_VkSetLayouts.push_back(l_Handle);
        }

        std::vector<VkPushConstantRange> l_PushRanges;
        l_PushRanges.reserve(specification.PushConstants.size());
        for (const auto& it_Range : specification.PushConstants)
        {
            VkPushConstantRange l_Range{};
            l_Range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            l_Range.offset = it_Range.Offset;
            l_Range.size = it_Range.Size;
            l_PushRanges.push_back(l_Range);
        }

        VkPipelineLayoutCreateInfo l_PipelineLayoutInfo{};
        l_PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_PipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(l_VkSetLayouts.size());
        l_PipelineLayoutInfo.pSetLayouts = l_VkSetLayouts.empty() ? nullptr : l_VkSetLayouts.data();
        l_PipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(l_PushRanges.size());
        l_PipelineLayoutInfo.pPushConstantRanges = l_PushRanges.empty() ? nullptr : l_PushRanges.data();

        VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device, &l_PipelineLayoutInfo, nullptr, &m_PipelineLayout), "Failed vkCreatePipelineLayout (compute)");

        VkComputePipelineCreateInfo l_PipelineInfo{};
        l_PipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        l_PipelineInfo.stage = a_ShaderStages[0];
        l_PipelineInfo.layout = m_PipelineLayout;

        VulkanUtilities::VKCheck(vkCreateComputePipelines(m_Device, pipelineCache, 1, &l_PipelineInfo, nullptr, &m_Pipeline), "Failed vkCreateComputePipelines");

        a_VulkanShader->ReleaseModules();
    }

    VulkanComputePipeline::~VulkanComputePipeline()
    {
        if (m_Pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
        }

        if (m_PipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
        }
    }
}