#include "Trinity/Renderer/Vulkan/Resources/VulkanPipeline.h"

#include "Trinity/Renderer/Vulkan/Resources/VulkanShader.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

#include <vector>

namespace Trinity
{
    namespace
    {
        VkBlendFactor ToVkBlendFactor(BlendFactor factor)
        {
            switch (factor)
            {
                case BlendFactor::Zero:
                    return VK_BLEND_FACTOR_ZERO;
                case BlendFactor::One:
                    return VK_BLEND_FACTOR_ONE;
                case BlendFactor::SrcColor:
                    return VK_BLEND_FACTOR_SRC_COLOR;
                case BlendFactor::OneMinusSrcColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case BlendFactor::DstColor:
                    return VK_BLEND_FACTOR_DST_COLOR;
                case BlendFactor::OneMinusDstColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                case BlendFactor::SrcAlpha:
                    return VK_BLEND_FACTOR_SRC_ALPHA;
                case BlendFactor::OneMinusSrcAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case BlendFactor::DstAlpha:
                    return VK_BLEND_FACTOR_DST_ALPHA;
                case BlendFactor::OneMinusDstAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case BlendFactor::ConstantColor:
                    return VK_BLEND_FACTOR_CONSTANT_COLOR;
                case BlendFactor::OneMinusConstantColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
                case BlendFactor::ConstantAlpha:
                    return VK_BLEND_FACTOR_CONSTANT_ALPHA;
                case BlendFactor::OneMinusConstantAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
                case BlendFactor::SrcAlphaSaturate:
                    return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
                default:
                    return VK_BLEND_FACTOR_ONE;
            }
        }

        VkBlendOp ToVkBlendOp(BlendOp op)
        {
            switch (op)
            {
                case BlendOp::Add:
                    return VK_BLEND_OP_ADD;
                case BlendOp::Subtract:
                    return VK_BLEND_OP_SUBTRACT;
                case BlendOp::ReverseSubtract:
                    return VK_BLEND_OP_REVERSE_SUBTRACT;
                case BlendOp::Min:
                    return VK_BLEND_OP_MIN;
                case BlendOp::Max:
                    return VK_BLEND_OP_MAX;
                default:
                    return VK_BLEND_OP_ADD;
            }
        }

        VkColorComponentFlags ToVkColorComponents(ColorWriteMask mask)
        {
            VkColorComponentFlags l_Flags = 0;
            if (mask & ColorWriteMask::Red)
            {
                l_Flags |= VK_COLOR_COMPONENT_R_BIT;
            }

            if (mask & ColorWriteMask::Green)
            {
                l_Flags |= VK_COLOR_COMPONENT_G_BIT;
            }

            if (mask & ColorWriteMask::Blue)
            {
                l_Flags |= VK_COLOR_COMPONENT_B_BIT;
            }

            if (mask & ColorWriteMask::Alpha)
            {
                l_Flags |= VK_COLOR_COMPONENT_A_BIT;
            }

            return l_Flags;
        }

        VkSampleCountFlagBits ToSampleCount(uint32_t samples)
        {
            switch (samples)
            {
                case 1:
                    return VK_SAMPLE_COUNT_1_BIT;
                case 2:
                    return VK_SAMPLE_COUNT_2_BIT;
                case 4:
                    return VK_SAMPLE_COUNT_4_BIT;
                case 8:
                    return VK_SAMPLE_COUNT_8_BIT;
                case 16:
                    return VK_SAMPLE_COUNT_16_BIT;
                case 32:
                    return VK_SAMPLE_COUNT_32_BIT;
                case 64:
                    return VK_SAMPLE_COUNT_64_BIT;
                default:
                    return VK_SAMPLE_COUNT_1_BIT;
            }
        }
    }

    VulkanPipeline::VulkanPipeline(VkDevice device, const PipelineSpecification& specification, VkPipelineCache pipelineCache) : m_Device(device)
    {
        m_Specification = specification;

        auto* a_VulkanShader = dynamic_cast<VulkanShader*>(specification.PipelineShader.get());
        if (a_VulkanShader == nullptr)
        {
            TR_CORE_ERROR("VulkanPipeline: shader is not a VulkanShader instance [{}]", specification.DebugName);

            return;
        }

        VkVertexInputBindingDescription l_BindingDescription{};
        l_BindingDescription.binding = 0;
        l_BindingDescription.stride = specification.VertexStride;
        l_BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> l_AttributeDescriptions;
        for (const auto& it_Attribute : specification.VertexAttributes)
        {
            VkVertexInputAttributeDescription l_Description{};
            l_Description.location = it_Attribute.Location;
            l_Description.binding = it_Attribute.Binding;
            l_Description.format = VulkanUtilities::ToVkFormat(it_Attribute.Format);
            l_Description.offset = it_Attribute.Offset;
            l_AttributeDescriptions.push_back(l_Description);
        }

        VkPipelineVertexInputStateCreateInfo l_VertexInputInfo{};
        l_VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        l_VertexInputInfo.vertexBindingDescriptionCount = specification.VertexStride > 0 ? 1 : 0;
        l_VertexInputInfo.pVertexBindingDescriptions = specification.VertexStride > 0 ? &l_BindingDescription : nullptr;
        l_VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(l_AttributeDescriptions.size());
        l_VertexInputInfo.pVertexAttributeDescriptions = l_AttributeDescriptions.empty() ? nullptr : l_AttributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo l_InputAssembly{};
        l_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        l_InputAssembly.topology = VulkanUtilities::ToVkTopology(specification.Topology);
        l_InputAssembly.primitiveRestartEnable = VK_FALSE;

        std::vector<VkDynamicState> l_DynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        if (specification.DepthBias)
        {
            l_DynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
        }

        VkPipelineDynamicStateCreateInfo l_DynamicState{};
        l_DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        l_DynamicState.dynamicStateCount = static_cast<uint32_t>(l_DynamicStates.size());
        l_DynamicState.pDynamicStates = l_DynamicStates.data();

        VkPipelineViewportStateCreateInfo l_ViewportState{};
        l_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        l_ViewportState.viewportCount = 1;
        l_ViewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo l_Rasterizer{};
        l_Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        l_Rasterizer.depthClampEnable = VK_FALSE;
        l_Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        l_Rasterizer.polygonMode = specification.WireframeMode ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        l_Rasterizer.lineWidth = 1.0f;
        l_Rasterizer.cullMode = VulkanUtilities::ToVkCullMode(specification.CullingMode);
        l_Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        l_Rasterizer.depthBiasEnable = specification.DepthBias ? VK_TRUE : VK_FALSE;
        l_Rasterizer.depthBiasConstantFactor = specification.DepthBiasConstantFactor;
        l_Rasterizer.depthBiasSlopeFactor = specification.DepthBiasSlopeFactor;

        VkPipelineMultisampleStateCreateInfo l_Multisampling{};
        l_Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        l_Multisampling.rasterizationSamples = ToSampleCount(specification.SampleCount);
        l_Multisampling.sampleShadingEnable = specification.SampleShadingEnable ? VK_TRUE : VK_FALSE;
        l_Multisampling.minSampleShading = specification.MinSampleShading;

        VkPipelineDepthStencilStateCreateInfo l_DepthStencil{};
        l_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        l_DepthStencil.depthTestEnable = specification.DepthTest ? VK_TRUE : VK_FALSE;
        l_DepthStencil.depthWriteEnable = specification.DepthWrite ? VK_TRUE : VK_FALSE;
        l_DepthStencil.depthCompareOp = VulkanUtilities::ToVkCompareOp(specification.DepthOp);
        l_DepthStencil.depthBoundsTestEnable = VK_FALSE;
        l_DepthStencil.stencilTestEnable = VK_FALSE;

        const size_t l_AttachmentCount = specification.ColorAttachmentFormats.size();
        std::vector<VkPipelineColorBlendAttachmentState> l_ColorBlendAttachments(l_AttachmentCount);

        const bool l_UsePerAttachmentBlend = !specification.BlendStates.empty();
        if (l_UsePerAttachmentBlend && specification.BlendStates.size() != l_AttachmentCount)
        {
            TR_CORE_WARN("VulkanPipeline [{}]: BlendStates count ({}) does not match ColorAttachmentFormats count ({}); falling back to opaque defaults", specification.DebugName, specification.BlendStates.size(), l_AttachmentCount);
        }

        for (size_t i = 0; i < l_AttachmentCount; i++)
        {
            VkPipelineColorBlendAttachmentState& a_Out = l_ColorBlendAttachments[i];

            if (l_UsePerAttachmentBlend && i < specification.BlendStates.size())
            {
                const BlendAttachmentState& a_In = specification.BlendStates[i];
                a_Out.blendEnable = a_In.BlendEnable ? VK_TRUE : VK_FALSE;
                a_Out.srcColorBlendFactor = ToVkBlendFactor(a_In.SrcColorFactor);
                a_Out.dstColorBlendFactor = ToVkBlendFactor(a_In.DstColorFactor);
                a_Out.colorBlendOp = ToVkBlendOp(a_In.ColorOp);
                a_Out.srcAlphaBlendFactor = ToVkBlendFactor(a_In.SrcAlphaFactor);
                a_Out.dstAlphaBlendFactor = ToVkBlendFactor(a_In.DstAlphaFactor);
                a_Out.alphaBlendOp = ToVkBlendOp(a_In.AlphaOp);
                a_Out.colorWriteMask = ToVkColorComponents(a_In.WriteMask);
            }
            else
            {
                a_Out.blendEnable = VK_FALSE;
                a_Out.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            }
        }

        VkPipelineColorBlendStateCreateInfo l_ColorBlending{};
        l_ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        l_ColorBlending.logicOpEnable = VK_FALSE;
        l_ColorBlending.attachmentCount = static_cast<uint32_t>(l_ColorBlendAttachments.size());
        l_ColorBlending.pAttachments = l_ColorBlendAttachments.empty() ? nullptr : l_ColorBlendAttachments.data();

        std::vector<VkPushConstantRange> l_PushRanges;
        for (const auto& it_Range : specification.PushConstants)
        {
            VkPushConstantRange l_Range{};
            l_Range.stageFlags = VulkanUtilities::ToVkShaderStage(it_Range.Stage);
            l_Range.offset = it_Range.Offset;
            l_Range.size = it_Range.Size;
            l_PushRanges.push_back(l_Range);
        }

        std::vector<VkDescriptorSetLayout> l_VkSetLayouts;
        l_VkSetLayouts.reserve(specification.DescriptorSetLayouts.size());
        for (const auto& it_Layout : specification.DescriptorSetLayouts)
        {
            if (!it_Layout)
            {
                TR_CORE_ERROR("VulkanPipeline [{}]: null entry in DescriptorSetLayouts; aborting layout creation", specification.DebugName);

                return;
            }

            VkDescriptorSetLayout l_Handle = reinterpret_cast<VkDescriptorSetLayout>(it_Layout->GetOpaqueHandle());
            if (l_Handle == VK_NULL_HANDLE)
            {
                TR_CORE_ERROR("VulkanPipeline [{}]: descriptor set layout has null Vulkan handle; aborting", specification.DebugName);

                return;
            }

            l_VkSetLayouts.push_back(l_Handle);
        }

        VkPipelineLayoutCreateInfo l_PipelineLayoutInfo{};
        l_PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_PipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(l_VkSetLayouts.size());
        l_PipelineLayoutInfo.pSetLayouts = l_VkSetLayouts.empty() ? nullptr : l_VkSetLayouts.data();
        l_PipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(l_PushRanges.size());
        l_PipelineLayoutInfo.pPushConstantRanges = l_PushRanges.empty() ? nullptr : l_PushRanges.data();

        VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device, &l_PipelineLayoutInfo, nullptr, &m_PipelineLayout), "Failed vkCreatePipelineLayout");

        std::vector<VkFormat> l_ColorFormats;
        l_ColorFormats.reserve(specification.ColorAttachmentFormats.size());
        for (const auto& it_Format : specification.ColorAttachmentFormats)
        {
            l_ColorFormats.push_back(VulkanUtilities::ToVkFormat(it_Format));
        }

        if (l_ColorFormats.empty())
        {
            l_ColorFormats.push_back(VK_FORMAT_B8G8R8A8_SRGB);
        }

        VkPipelineRenderingCreateInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        l_RenderingInfo.colorAttachmentCount = static_cast<uint32_t>(l_ColorFormats.size());
        l_RenderingInfo.pColorAttachmentFormats = l_ColorFormats.data();

        if (specification.DepthAttachmentFormat != TextureFormat::None)
        {
            l_RenderingInfo.depthAttachmentFormat = VulkanUtilities::ToVkFormat(specification.DepthAttachmentFormat);

            const bool l_HasStencil = specification.DepthAttachmentFormat == TextureFormat::Depth24Stencil8 || specification.DepthAttachmentFormat == TextureFormat::Depth32FStencil8;
            if (l_HasStencil)
            {
                l_RenderingInfo.stencilAttachmentFormat = VulkanUtilities::ToVkFormat(specification.DepthAttachmentFormat);
            }
        }

        const auto& a_ShaderStages = a_VulkanShader->GetStageCreateInfos();

        VkGraphicsPipelineCreateInfo l_PipelineInfo{};
        l_PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        l_PipelineInfo.pNext = &l_RenderingInfo;
        l_PipelineInfo.stageCount = static_cast<uint32_t>(a_ShaderStages.size());
        l_PipelineInfo.pStages = a_ShaderStages.data();
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

        VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device, pipelineCache, 1, &l_PipelineInfo, nullptr, &m_Pipeline), "Failed vkCreateGraphicsPipelines");

        a_VulkanShader->ReleaseModules();
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