#include "Trinity/Renderer/Vulkan/Resources/VulkanPipeline.h"

#include "Trinity/Renderer/Vulkan/Resources/VulkanShader.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

#include <vector>

namespace Trinity
{
    VulkanPipeline::VulkanPipeline(VkDevice device, const PipelineSpecification& specification) : m_Device(device)
    {
        m_Specification = specification;

        auto* a_VulkanShader = dynamic_cast<VulkanShader*>(specification.PipelineShader.get());
        if (a_VulkanShader == nullptr)
        {
            TR_CORE_ERROR("VulkanPipeline requires a VulkanShader.");
            return;
        }

        // Vertex input
        VkVertexInputBindingDescription l_BindingDescriptions{};
        l_BindingDescriptions.binding = 0;
        l_BindingDescriptions.stride = specification.VertexStride;
        l_BindingDescriptions.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> l_AttributeDescriptions;
        for (const auto& it_Attribute : specification.VertexAttributes)
        {
            VkVertexInputAttributeDescription l_Description{};
            l_Description.location = it_Attribute.Location;
            l_Description.binding = it_Attribute.Binding;
            l_Description.format = VulkanUtilities::ToVkVertexFormat(it_Attribute.Format);
            l_Description.offset = it_Attribute.Offset;
            l_AttributeDescriptions.push_back(l_Description);
        }

        VkPipelineVertexInputStateCreateInfo l_VertexInputInfo{};
        l_VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        if (!specification.VertexAttributes.empty())
        {
            l_VertexInputInfo.vertexBindingDescriptionCount = 1;
            l_VertexInputInfo.pVertexBindingDescriptions = &l_BindingDescriptions;
            l_VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(l_AttributeDescriptions.size());
            l_VertexInputInfo.pVertexAttributeDescriptions = l_AttributeDescriptions.data();
        }

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo l_InputAssembly{};
        l_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        l_InputAssembly.topology = VulkanUtilities::ToVkTopology(specification.Topology);
        l_InputAssembly.primitiveRestartEnable = VK_FALSE;

        // Dynamic state (viewport + scissor)
        std::vector<VkDynamicState> l_DynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo l_DynamicState{};
        l_DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        l_DynamicState.dynamicStateCount = static_cast<uint32_t>(l_DynamicStates.size());
        l_DynamicState.pDynamicStates = l_DynamicStates.data();

        // Viewport
        VkPipelineViewportStateCreateInfo l_ViewportState{};
        l_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        l_ViewportState.viewportCount = 1;
        l_ViewportState.scissorCount = 1;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo l_Rasterizer{};
        l_Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        l_Rasterizer.depthClampEnable = VK_FALSE;
        l_Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        l_Rasterizer.polygonMode = specification.WireframeMode ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        l_Rasterizer.lineWidth = 1.0f;
        l_Rasterizer.cullMode = VulkanUtilities::ToVkCullMode(specification.CullingMode);
        l_Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;  // Y-flip in projection (P[1][1]*=-1) inverts apparent winding
        l_Rasterizer.depthBiasEnable = specification.DepthBias ? VK_TRUE : VK_FALSE;
        l_Rasterizer.depthBiasConstantFactor = specification.DepthBiasConstantFactor;
        l_Rasterizer.depthBiasSlopeFactor = specification.DepthBiasSlopeFactor;

        // Multisampling
        VkPipelineMultisampleStateCreateInfo l_Multisampling{};
        l_Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        l_Multisampling.sampleShadingEnable = VK_FALSE;
        l_Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Depth stencil
        VkPipelineDepthStencilStateCreateInfo l_DepthStencil{};
        l_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        l_DepthStencil.depthTestEnable = specification.DepthTest ? VK_TRUE : VK_FALSE;
        l_DepthStencil.depthWriteEnable = specification.DepthWrite ? VK_TRUE : VK_FALSE;
        l_DepthStencil.depthCompareOp = VulkanUtilities::ToVkCompareOp(specification.DepthOp);
        l_DepthStencil.depthBoundsTestEnable = VK_FALSE;
        l_DepthStencil.stencilTestEnable = VK_FALSE;

        // Color blend attachments
        std::vector<VkPipelineColorBlendAttachmentState> l_ColorBlendAttachments;
        for (size_t i = 0; i < specification.ColorAttachmentFormats.size(); i++)
        {
            VkPipelineColorBlendAttachmentState l_Attachment{};
            l_Attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            l_Attachment.blendEnable = VK_FALSE;
            l_ColorBlendAttachments.push_back(l_Attachment);
        }

        VkPipelineColorBlendStateCreateInfo l_ColorBlending{};
        l_ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        l_ColorBlending.logicOpEnable = VK_FALSE;
        l_ColorBlending.attachmentCount = static_cast<uint32_t>(l_ColorBlendAttachments.size());
        l_ColorBlending.pAttachments = l_ColorBlendAttachments.empty() ? nullptr : l_ColorBlendAttachments.data();

        // Pipeline layout
        std::vector<VkPushConstantRange> l_PushRanges;
        for (const auto& it_Range : specification.PushConstants)
        {
            VkPushConstantRange l_Range{};
            l_Range.stageFlags = VulkanUtilities::ToVkShaderStage(it_Range.Stage);
            l_Range.offset = it_Range.Offset;
            l_Range.size = it_Range.Size;
            l_PushRanges.push_back(l_Range);
        }

        VkPipelineLayoutCreateInfo l_PipelineLayoutInfo{};
        l_PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_PipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(l_PushRanges.size());
        l_PipelineLayoutInfo.pPushConstantRanges = l_PushRanges.empty() ? nullptr : l_PushRanges.data();

        VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device, &l_PipelineLayoutInfo, nullptr, &m_PipelineLayout), "Failed vkCreatePipelineLayout");

        // Dynamic rendering format info
        std::vector<VkFormat> l_ColorFormats;
        for (const auto& l_Fmt : specification.ColorAttachmentFormats)
        {
            l_ColorFormats.push_back(VulkanUtilities::ToVkFormat(l_Fmt));
        }

        VkPipelineRenderingCreateInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        l_RenderingInfo.colorAttachmentCount = static_cast<uint32_t>(l_ColorFormats.size());
        l_RenderingInfo.pColorAttachmentFormats = l_ColorFormats.empty() ? nullptr : l_ColorFormats.data();

        if (specification.DepthAttachmentFormat != TextureFormat::None)
        {
            l_RenderingInfo.depthAttachmentFormat = VulkanUtilities::ToVkFormat(specification.DepthAttachmentFormat);

            if (specification.DepthAttachmentFormat == TextureFormat::Depth24Stencil8)
            {
                l_RenderingInfo.stencilAttachmentFormat = VulkanUtilities::ToVkFormat(specification.DepthAttachmentFormat);
            }
        }

        // Create pipeline
        const auto& l_ShaderStages = a_VulkanShader->GetStageCreateInfos();

        VkGraphicsPipelineCreateInfo l_PipelineInfo{};
        l_PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        l_PipelineInfo.pNext = &l_RenderingInfo;
        l_PipelineInfo.stageCount = static_cast<uint32_t>(l_ShaderStages.size());
        l_PipelineInfo.pStages = l_ShaderStages.data();
        l_PipelineInfo.pVertexInputState = &l_VertexInputInfo;
        l_PipelineInfo.pInputAssemblyState = &l_InputAssembly;
        l_PipelineInfo.pViewportState = &l_ViewportState;
        l_PipelineInfo.pRasterizationState = &l_Rasterizer;
        l_PipelineInfo.pMultisampleState = &l_Multisampling;
        l_PipelineInfo.pDepthStencilState = &l_DepthStencil;
        l_PipelineInfo.pColorBlendState = &l_ColorBlending;
        l_PipelineInfo.pDynamicState = &l_DynamicState;
        l_PipelineInfo.layout = m_PipelineLayout;
        l_PipelineInfo.renderPass = VK_NULL_HANDLE;

        VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &l_PipelineInfo, nullptr, &m_Pipeline), "Failed vkCreateGraphicsPipelines");
    }

    VulkanPipeline::~VulkanPipeline()
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