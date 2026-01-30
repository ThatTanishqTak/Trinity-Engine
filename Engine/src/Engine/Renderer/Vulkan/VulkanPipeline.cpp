#include "Engine/Renderer/Vulkan/VulkanPipeline.h"

#include "Engine/Utilities/Utilities.h"

#include <array>

namespace Engine
{
    void VulkanPipeline::Initialize(VkDevice device)
    {
        m_Device = device;
    }

    void VulkanPipeline::Shutdown()
    {
        Cleanup();
        m_Device = VK_NULL_HANDLE;
    }

    void VulkanPipeline::Cleanup()
    {
        if (m_Pipeline)
        {
            vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
            m_Pipeline = VK_NULL_HANDLE;
        }

        if (m_PipelineLayout)
        {
            vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
        }
    }

    VkShaderModule VulkanPipeline::CreateShaderModule(const std::vector<char>& code) const
    {
        VkShaderModuleCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        l_Info.codeSize = code.size();
        l_Info.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule l_Module = VK_NULL_HANDLE;
        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateShaderModule(m_Device, &l_Info, nullptr, &l_Module), "vkCreateShaderModule");

        return l_Module;
    }

    void VulkanPipeline::CreateGraphicsPipeline(VkRenderPass renderPass, const VkVertexInputBindingDescription& bindingDescription,
        const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, const char* vertexSpvPath, const char* fragmentSpvPath,
        bool depthEnabled, const std::vector<VkDescriptorSetLayout>& setLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges)
    {
        Cleanup();

        const std::vector<char> l_VertCode = Engine::Utilities::FileManagement::LoadFromFile(vertexSpvPath);
        const std::vector<char> l_FragCode = Engine::Utilities::FileManagement::LoadFromFile(fragmentSpvPath);

        VkShaderModule l_VertModule = CreateShaderModule(l_VertCode);
        VkShaderModule l_FragModule = CreateShaderModule(l_FragCode);

        VkPipelineShaderStageCreateInfo l_VertStage{};
        l_VertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_VertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        l_VertStage.module = l_VertModule;
        l_VertStage.pName = "main";

        VkPipelineShaderStageCreateInfo l_FragStage{};
        l_FragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_FragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        l_FragStage.module = l_FragModule;
        l_FragStage.pName = "main";

        VkPipelineShaderStageCreateInfo l_Stages[] = { l_VertStage, l_FragStage };

        VkPipelineVertexInputStateCreateInfo l_VertexInput{};
        l_VertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        l_VertexInput.vertexBindingDescriptionCount = 1;
        l_VertexInput.pVertexBindingDescriptions = &bindingDescription;
        l_VertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        l_VertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo l_InputAssembly{};
        l_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        l_InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        l_InputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo l_ViewportState{};
        l_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        l_ViewportState.viewportCount = 1;
        l_ViewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo l_Raster{};
        l_Raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        l_Raster.depthClampEnable = VK_FALSE;
        l_Raster.rasterizerDiscardEnable = VK_FALSE;
        l_Raster.polygonMode = VK_POLYGON_MODE_FILL;
        l_Raster.lineWidth = 1.0f;
        l_Raster.cullMode = VK_CULL_MODE_NONE;
        l_Raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
        l_Raster.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo l_MS{};
        l_MS.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        l_MS.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        l_MS.sampleShadingEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState l_ColorBlendAtt{};
        l_ColorBlendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        l_ColorBlendAtt.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo l_ColorBlend{};
        l_ColorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        l_ColorBlend.logicOpEnable = VK_FALSE;
        l_ColorBlend.attachmentCount = 1;
        l_ColorBlend.pAttachments = &l_ColorBlendAtt;

        VkPipelineDepthStencilStateCreateInfo l_DepthStencil{};
        l_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        l_DepthStencil.depthTestEnable = depthEnabled ? VK_TRUE : VK_FALSE;
        l_DepthStencil.depthWriteEnable = depthEnabled ? VK_TRUE : VK_FALSE;
        l_DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        l_DepthStencil.depthBoundsTestEnable = VK_FALSE;
        l_DepthStencil.stencilTestEnable = VK_FALSE;

        std::array<VkDynamicState, 2> l_DynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo l_Dynamic{};
        l_Dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        l_Dynamic.dynamicStateCount = static_cast<uint32_t>(l_DynamicStates.size());
        l_Dynamic.pDynamicStates = l_DynamicStates.data();

        VkPipelineLayoutCreateInfo l_LayoutInfo{};
        l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_LayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        l_LayoutInfo.pSetLayouts = setLayouts.empty() ? nullptr : setLayouts.data();
        l_LayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
        l_LayoutInfo.pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data();

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreatePipelineLayout(m_Device, &l_LayoutInfo, nullptr, &m_PipelineLayout), "vkCreatePipelineLayout");

        VkGraphicsPipelineCreateInfo l_PipelineInfo{};
        l_PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        l_PipelineInfo.stageCount = 2;
        l_PipelineInfo.pStages = l_Stages;
        l_PipelineInfo.pVertexInputState = &l_VertexInput;
        l_PipelineInfo.pInputAssemblyState = &l_InputAssembly;
        l_PipelineInfo.pViewportState = &l_ViewportState;
        l_PipelineInfo.pRasterizationState = &l_Raster;
        l_PipelineInfo.pMultisampleState = &l_MS;
        l_PipelineInfo.pColorBlendState = &l_ColorBlend;
        l_PipelineInfo.pDepthStencilState = depthEnabled ? &l_DepthStencil : nullptr;
        l_PipelineInfo.pDynamicState = &l_Dynamic;
        l_PipelineInfo.layout = m_PipelineLayout;
        l_PipelineInfo.renderPass = renderPass;
        l_PipelineInfo.subpass = 0;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &l_PipelineInfo, nullptr, &m_Pipeline), "vkCreateGraphicsPipelines");

        vkDestroyShaderModule(m_Device, l_FragModule, nullptr);
        vkDestroyShaderModule(m_Device, l_VertModule, nullptr);
    }
}