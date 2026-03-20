#include "Trinity/Renderer/Vulkan/Passes/VulkanPostProcessPass.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <cstdlib>

namespace Trinity
{
    void VulkanPostProcessPass::Initialize(const VulkanPassContext& context)
    {
        TR_CORE_TRACE("Initializing VulkanPostProcessPass");

        CopyContext(context);

        m_DescriptorAllocator.Initialize(m_Device, m_HostAllocator, m_FramesInFlight, 4);

        CreatePipeline();

        // Output image is created on first Recreate call.

        TR_CORE_TRACE("VulkanPostProcessPass Initialized");
    }

    void VulkanPostProcessPass::Shutdown()
    {
        TR_CORE_TRACE("Shutting Down VulkanPostProcessPass");

        DestroyOutputImage();
        DestroyPipeline();
        m_DescriptorAllocator.Shutdown();

        TR_CORE_TRACE("VulkanPostProcessPass Shutdown Complete");
    }

    void VulkanPostProcessPass::Recreate(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        DestroyOutputImage();
        CreateOutputImage(width, height);

        TR_CORE_TRACE("VulkanPostProcessPass Recreated ({}x{})", width, height);
    }

    void VulkanPostProcessPass::SetSceneColorResources(VkImageView sceneColorView, VkSampler sceneColorSampler)
    {
        m_SceneColorView = sceneColorView;
        m_SceneColorSampler = sceneColorSampler;
    }

    void VulkanPostProcessPass::Execute(const VulkanFrameContext& frameContext)
    {
        if (m_PostProcessImage == VK_NULL_HANDLE || m_SceneColorView == VK_NULL_HANDLE)
        {
            return;
        }

        const VkCommandBuffer l_CB = frameContext.CommandBuffer;
        const uint32_t l_Width = frameContext.ViewportWidth;
        const uint32_t l_Height = frameContext.ViewportHeight;

        m_DescriptorAllocator.BeginFrame(frameContext.FrameIndex);

        // Transition output image to COLOR_ATTACHMENT
        const VkImageSubresourceRange l_ColorRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        m_ResourceStateTracker->TransitionImageResource(l_CB, m_PostProcessImage, l_ColorRange, g_ColorAttachmentWriteImageState);

        VkClearValue l_Clear{};
        VkRenderingAttachmentInfo l_ColorAtt{};
        l_ColorAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        l_ColorAtt.imageView = m_PostProcessImageView;
        l_ColorAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        l_ColorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        l_ColorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        l_ColorAtt.clearValue = l_Clear;

        VkRenderingInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        l_RenderingInfo.renderArea = { { 0, 0 }, { l_Width, l_Height } };
        l_RenderingInfo.layerCount = 1;
        l_RenderingInfo.colorAttachmentCount = 1;
        l_RenderingInfo.pColorAttachments = &l_ColorAtt;

        vkCmdBeginRendering(l_CB, &l_RenderingInfo);

        VkViewport l_Viewport{ 0.0f, 0.0f, static_cast<float>(l_Width), static_cast<float>(l_Height), 0.0f, 1.0f };
        VkRect2D   l_Scissor{ { 0, 0 }, { l_Width, l_Height } };
        vkCmdSetViewport(l_CB, 0, 1, &l_Viewport);
        vkCmdSetScissor(l_CB, 0, 1, &l_Scissor);
        vkCmdBindPipeline(l_CB, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PostProcessPipeline);

        // Allocate and write one texture descriptor set
        const VkDescriptorSet l_Set = m_DescriptorAllocator.Allocate(frameContext.FrameIndex, m_PostProcessSetLayout);

        VkDescriptorImageInfo l_ImageInfo{};
        l_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        l_ImageInfo.imageView = m_SceneColorView;
        l_ImageInfo.sampler = m_SceneColorSampler;

        VkWriteDescriptorSet l_Write{};
        l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_Write.dstSet = l_Set;
        l_Write.dstBinding = 0;
        l_Write.descriptorCount = 1;
        l_Write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_Write.pImageInfo = &l_ImageInfo;
        vkUpdateDescriptorSets(m_Device, 1, &l_Write, 0, nullptr);

        vkCmdBindDescriptorSets(l_CB, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PostProcessPipelineLayout, 0, 1, &l_Set, 0, nullptr);

        vkCmdDraw(l_CB, 3, 1, 0, 0);
        vkCmdEndRendering(l_CB);

        // Transition to SHADER_READ_ONLY so ImGui can sample it
        m_ResourceStateTracker->TransitionImageResource(l_CB, m_PostProcessImage, l_ColorRange, g_ShaderReadOnlyImageState);
    }

    // -----------------------------------------------------------------

    void VulkanPostProcessPass::CreateOutputImage(uint32_t width, uint32_t height)
    {
        VkImageCreateInfo l_ImageInfo{};
        l_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        l_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
        l_ImageInfo.extent = { width, height, 1 };
        l_ImageInfo.mipLevels = 1;
        l_ImageInfo.arrayLayers = 1;
        l_ImageInfo.format = m_SwapchainFormat;
        l_ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        l_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        l_ImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        l_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        l_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        m_Allocator->CreateImage(l_ImageInfo, m_PostProcessImage, m_PostProcessImageAllocation);

        VkImageViewCreateInfo l_ViewInfo{};
        l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_ViewInfo.image = m_PostProcessImage;
        l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        l_ViewInfo.format = m_SwapchainFormat;
        l_ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        l_ViewInfo.subresourceRange.levelCount = 1;
        l_ViewInfo.subresourceRange.layerCount = 1;
        Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, m_HostAllocator, &m_PostProcessImageView), "VulkanPostProcessPass: vkCreateImageView failed");

        VkSamplerCreateInfo l_SamplerInfo{};
        l_SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        l_SamplerInfo.magFilter = VK_FILTER_LINEAR;
        l_SamplerInfo.minFilter = VK_FILTER_LINEAR;
        l_SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        l_SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerInfo.maxLod = 0.0f;
        l_SamplerInfo.maxAnisotropy = 1.0f;
        Utilities::VulkanUtilities::VKCheck(vkCreateSampler(m_Device, &l_SamplerInfo, m_HostAllocator, &m_PostProcessSampler), "VulkanPostProcessPass: vkCreateSampler failed");

        m_PostProcessHandle = ImGui_ImplVulkan_AddTexture(m_PostProcessSampler, m_PostProcessImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void VulkanPostProcessPass::DestroyOutputImage()
    {
        if (m_PostProcessHandle != nullptr)
        {
            if (ImGui::GetCurrentContext() != nullptr)
            {
                ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(m_PostProcessHandle));
            }
            m_PostProcessHandle = nullptr;
        }

        if (m_PostProcessSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_Device, m_PostProcessSampler, m_HostAllocator);
            m_PostProcessSampler = VK_NULL_HANDLE;
        }

        if (m_PostProcessImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, m_PostProcessImageView, m_HostAllocator);
            m_PostProcessImageView = VK_NULL_HANDLE;
        }

        if (m_PostProcessImage != VK_NULL_HANDLE)
        {
            if (m_ResourceStateTracker)
            {
                m_ResourceStateTracker->ForgetImage(m_PostProcessImage);
            }
            m_Allocator->DestroyImage(m_PostProcessImage, m_PostProcessImageAllocation);
            m_PostProcessImage = VK_NULL_HANDLE;
            m_PostProcessImageAllocation = VK_NULL_HANDLE;
        }
    }

    void VulkanPostProcessPass::CreatePipeline()
    {
        m_Shaders->Load("PostProcess.vert", "Assets/Shaders/PostProcess.vert.spv", ShaderStage::Vertex);
        m_Shaders->Load("PostProcess.frag", "Assets/Shaders/PostProcess.frag.spv", ShaderStage::Fragment);

        const std::vector<uint32_t>& l_VertSpirV = *m_Shaders->GetSpirV("PostProcess.vert");
        const std::vector<uint32_t>& l_FragSpirV = *m_Shaders->GetSpirV("PostProcess.frag");

        VkDescriptorSetLayoutBinding l_Binding{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
        VkDescriptorSetLayoutCreateInfo l_SetLayoutInfo{};
        l_SetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        l_SetLayoutInfo.bindingCount = 1;
        l_SetLayoutInfo.pBindings = &l_Binding;
        Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device, &l_SetLayoutInfo, m_HostAllocator, &m_PostProcessSetLayout), "VulkanPostProcessPass: vkCreateDescriptorSetLayout failed");

        VkPipelineLayoutCreateInfo l_LayoutInfo{};
        l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_LayoutInfo.setLayoutCount = 1;
        l_LayoutInfo.pSetLayouts = &m_PostProcessSetLayout;
        Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device, &l_LayoutInfo, m_HostAllocator, &m_PostProcessPipelineLayout), "VulkanPostProcessPass: vkCreatePipelineLayout failed");

        auto a_CreateModule = [&](const std::vector<uint32_t>& spv) -> VkShaderModule
        {
            VkShaderModuleCreateInfo l_Info{};
            l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            l_Info.codeSize = spv.size() * 4;
            l_Info.pCode = spv.data();
            VkShaderModule l_Module = VK_NULL_HANDLE;
            Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device, &l_Info, m_HostAllocator, &l_Module), "VulkanPostProcessPass: vkCreateShaderModule failed");
            return l_Module;
        };

        const VkShaderModule l_VertModule = a_CreateModule(l_VertSpirV);
        const VkShaderModule l_FragModule = a_CreateModule(l_FragSpirV);

        VkPipelineShaderStageCreateInfo l_Stages[2]{};
        l_Stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT,   l_VertModule, "main" };
        l_Stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, l_FragModule, "main" };

        VkPipelineVertexInputStateCreateInfo l_VertexInput{};
        l_VertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo l_InputAssembly{};
        l_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        l_InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo l_ViewportState{};
        l_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        l_ViewportState.viewportCount = 1;
        l_ViewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo l_Raster{};
        l_Raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        l_Raster.polygonMode = VK_POLYGON_MODE_FILL;
        l_Raster.cullMode = VK_CULL_MODE_NONE;
        l_Raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        l_Raster.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo l_MSAA{};
        l_MSAA.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        l_MSAA.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo l_DepthStencil{};
        l_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        VkPipelineColorBlendAttachmentState l_BlendAtt{};
        l_BlendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo l_Blend{};
        l_Blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        l_Blend.attachmentCount = 1;
        l_Blend.pAttachments = &l_BlendAtt;

        const VkDynamicState l_DynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo l_Dynamic{};
        l_Dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        l_Dynamic.dynamicStateCount = 2;
        l_Dynamic.pDynamicStates = l_DynStates;

        VkPipelineRenderingCreateInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        l_RenderingInfo.colorAttachmentCount = 1;
        l_RenderingInfo.pColorAttachmentFormats = &m_SwapchainFormat;

        VkGraphicsPipelineCreateInfo l_PipelineInfo{};
        l_PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        l_PipelineInfo.pNext = &l_RenderingInfo;
        l_PipelineInfo.stageCount = 2;
        l_PipelineInfo.pStages = l_Stages;
        l_PipelineInfo.pVertexInputState = &l_VertexInput;
        l_PipelineInfo.pInputAssemblyState = &l_InputAssembly;
        l_PipelineInfo.pViewportState = &l_ViewportState;
        l_PipelineInfo.pRasterizationState = &l_Raster;
        l_PipelineInfo.pMultisampleState = &l_MSAA;
        l_PipelineInfo.pDepthStencilState = &l_DepthStencil;
        l_PipelineInfo.pColorBlendState = &l_Blend;
        l_PipelineInfo.pDynamicState = &l_Dynamic;
        l_PipelineInfo.layout = m_PostProcessPipelineLayout;

        Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &l_PipelineInfo, m_HostAllocator, &m_PostProcessPipeline), "VulkanPostProcessPass: vkCreateGraphicsPipelines failed");

        vkDestroyShaderModule(m_Device, l_VertModule, m_HostAllocator);
        vkDestroyShaderModule(m_Device, l_FragModule, m_HostAllocator);
    }

    void VulkanPostProcessPass::DestroyPipeline()
    {
        if (m_PostProcessPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_Device, m_PostProcessPipeline, m_HostAllocator);
            m_PostProcessPipeline = VK_NULL_HANDLE;
        }

        if (m_PostProcessPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_Device, m_PostProcessPipelineLayout, m_HostAllocator);
            m_PostProcessPipelineLayout = VK_NULL_HANDLE;
        }

        if (m_PostProcessSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_Device, m_PostProcessSetLayout, m_HostAllocator);
            m_PostProcessSetLayout = VK_NULL_HANDLE;
        }
    }
}