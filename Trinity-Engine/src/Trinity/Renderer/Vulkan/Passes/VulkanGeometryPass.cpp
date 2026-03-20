#include "Trinity/Renderer/Vulkan/Passes/VulkanGeometryPass.h"

#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/VulkanShaderInterop.h"
#include "Trinity/Geometry/Geometry.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>

namespace Trinity
{
    void VulkanGeometryPass::Initialize(const VulkanPassContext& context)
    {
        TR_CORE_TRACE("Initializing VulkanGeometryPass");

        CopyContext(context);

        m_DescriptorAllocator.Initialize(m_Device, m_HostAllocator, m_FramesInFlight, s_MaxTextureDescriptorsPerFrame);
        m_WhiteTexture.CreateSolid(*m_Allocator, *m_UploadContext, m_Device, m_HostAllocator, 255, 255, 255, 255);
        m_GBuffer.Initialize(m_Device, m_HostAllocator, *m_Allocator, 0, 0);

        // GBuffer images are allocated on the first Recreate call once a valid viewport size is known.
        CreatePipeline();

        TR_CORE_TRACE("VulkanGeometryPass Initialized");
    }

    void VulkanGeometryPass::Shutdown()
    {
        TR_CORE_TRACE("Shutting Down VulkanGeometryPass");

        DestroyPipeline();
        DestroyDepthImage();
        m_GBuffer.Shutdown();
        m_WhiteTexture.Destroy();
        m_DescriptorAllocator.Shutdown();

        TR_CORE_TRACE("VulkanGeometryPass Shutdown Complete");
    }

    void VulkanGeometryPass::Recreate(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        DestroyDepthImage();
        m_GBuffer.Recreate(width, height);
        CreateDepthImage(width, height);

        TR_CORE_TRACE("VulkanGeometryPass Recreated ({}x{})", width, height);
    }

    void VulkanGeometryPass::Reset()
    {
        m_DrawCommands.clear();
    }

    void VulkanGeometryPass::Submit(const GeometryDrawCommand& command)
    {
        m_DrawCommands.push_back(command);
    }

    void VulkanGeometryPass::Execute(const VulkanFrameContext& frameContext)
    {
        if (!m_GBuffer.IsValid() || m_DepthImageView == VK_NULL_HANDLE)
        {
            return;
        }

        const VkCommandBuffer l_CB = frameContext.CommandBuffer;
        const uint32_t l_Width = frameContext.ViewportWidth;
        const uint32_t l_Height = frameContext.ViewportHeight;

        m_DescriptorAllocator.BeginFrame(frameContext.FrameIndex);

        // Transition GBuffer attachments and depth to COLOR/DEPTH_ATTACHMENT
        const VkImageSubresourceRange l_ColorRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        const VkImageSubresourceRange l_DepthRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

        m_ResourceStateTracker->TransitionImageResource(l_CB, m_GBuffer.GetAlbedoImage(), l_ColorRange, g_ColorAttachmentWriteImageState);
        m_ResourceStateTracker->TransitionImageResource(l_CB, m_GBuffer.GetNormalImage(), l_ColorRange, g_ColorAttachmentWriteImageState);
        m_ResourceStateTracker->TransitionImageResource(l_CB, m_GBuffer.GetMaterialImage(), l_ColorRange, g_ColorAttachmentWriteImageState);
        m_ResourceStateTracker->TransitionImageResource(l_CB, m_DepthImage, l_DepthRange, g_DepthAttachmentWriteImageState);

        VkClearValue l_ClearColor{};
        VkRenderingAttachmentInfo l_ColorAtts[3]{};
        const VkImageView l_Views[3] = { m_GBuffer.GetAlbedoView(), m_GBuffer.GetNormalView(), m_GBuffer.GetMaterialView() };

        for (uint32_t i = 0; i < 3; ++i)
        {
            l_ColorAtts[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            l_ColorAtts[i].imageView = l_Views[i];
            l_ColorAtts[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            l_ColorAtts[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            l_ColorAtts[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            l_ColorAtts[i].clearValue = l_ClearColor;
        }

        VkClearValue l_ClearDepth{};
        l_ClearDepth.depthStencil = { 1.0f, 0 };

        VkRenderingAttachmentInfo l_DepthAtt{};
        l_DepthAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        l_DepthAtt.imageView = m_DepthImageView;
        l_DepthAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        l_DepthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        l_DepthAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        l_DepthAtt.clearValue = l_ClearDepth;

        VkRenderingInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        l_RenderingInfo.renderArea = { { 0, 0 }, { l_Width, l_Height } };
        l_RenderingInfo.layerCount = 1;
        l_RenderingInfo.colorAttachmentCount = 3;
        l_RenderingInfo.pColorAttachments = l_ColorAtts;
        l_RenderingInfo.pDepthAttachment = &l_DepthAtt;

        vkCmdBeginRendering(l_CB, &l_RenderingInfo);

        VkViewport l_Viewport{ 0.0f, 0.0f, static_cast<float>(l_Width), static_cast<float>(l_Height), 0.0f, 1.0f };
        VkRect2D l_Scissor{ { 0, 0 }, { l_Width, l_Height } };
        vkCmdSetViewport(l_CB, 0, 1, &l_Viewport);
        vkCmdSetScissor(l_CB, 0, 1, &l_Scissor);
        vkCmdBindPipeline(l_CB, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GBufferPipeline);

        for (const GeometryDrawCommand& it_Command : m_DrawCommands)
        {
            // Resolve albedo texture
            VkImageView l_View = m_WhiteTexture.GetImageView();
            VkSampler l_Sampler = m_WhiteTexture.GetSampler();

            if (it_Command.AlbedoTexture)
            {
                l_View = it_Command.AlbedoTexture->GetImageView();
                l_Sampler = it_Command.AlbedoTexture->GetSampler();
            }

            // Allocate and write texture descriptor set
            const VkDescriptorSet l_TexSet = m_DescriptorAllocator.Allocate(frameContext.FrameIndex, m_GBufferTextureSetLayout);

            VkDescriptorImageInfo l_ImageInfo{};
            l_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            l_ImageInfo.imageView = l_View;
            l_ImageInfo.sampler = l_Sampler;

            VkWriteDescriptorSet l_Write{};
            l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            l_Write.dstSet = l_TexSet;
            l_Write.dstBinding = 0;
            l_Write.descriptorCount = 1;
            l_Write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            l_Write.pImageInfo = &l_ImageInfo;
            vkUpdateDescriptorSets(m_Device, 1, &l_Write, 0, nullptr);

            vkCmdBindDescriptorSets(l_CB, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GBufferPipelineLayout, 0, 1, &l_TexSet, 0, nullptr);

            VkBuffer l_VkVB = reinterpret_cast<VkBuffer>(it_Command.VB->GetNativeHandle());
            VkBuffer l_VkIB = reinterpret_cast<VkBuffer>(it_Command.IB->GetNativeHandle());
            VkDeviceSize l_Offset = 0;

            vkCmdBindVertexBuffers(l_CB, 0, 1, &l_VkVB, &l_Offset);
            vkCmdBindIndexBuffer(l_CB, l_VkIB, 0, (it_Command.IB->GetIndexType() == IndexType::UInt16) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

            GeometryBufferPushConstants l_PC{};
            l_PC.ModelViewProjection = it_Command.ViewProjection * it_Command.Model;
            l_PC.Model = it_Command.Model;
            l_PC.Color = it_Command.Color;
            vkCmdPushConstants(l_CB, m_GBufferPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GeometryBufferPushConstants), &l_PC);

            vkCmdDrawIndexed(l_CB, it_Command.IndexCount, 1, 0, 0, 0);
        }

        vkCmdEndRendering(l_CB);

        // Transition GBuffer and depth to shader-read for the lighting pass
        m_ResourceStateTracker->TransitionImageResource(l_CB, m_GBuffer.GetAlbedoImage(), l_ColorRange, g_ShaderReadOnlyImageState);
        m_ResourceStateTracker->TransitionImageResource(l_CB, m_GBuffer.GetNormalImage(), l_ColorRange, g_ShaderReadOnlyImageState);
        m_ResourceStateTracker->TransitionImageResource(l_CB, m_GBuffer.GetMaterialImage(), l_ColorRange, g_ShaderReadOnlyImageState);
        m_ResourceStateTracker->TransitionImageResource(l_CB, m_DepthImage, l_DepthRange, g_DepthShaderReadOnlyImageState);
    }

    // -----------------------------------------------------------------

    void VulkanGeometryPass::CreateDepthImage(uint32_t width, uint32_t height)
    {
        VkImageCreateInfo l_ImageInfo{};
        l_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        l_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
        l_ImageInfo.extent = { width, height, 1 };
        l_ImageInfo.mipLevels = 1;
        l_ImageInfo.arrayLayers = 1;
        l_ImageInfo.format = m_DepthFormat;
        l_ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        l_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        l_ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        l_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        l_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        m_Allocator->CreateImage(l_ImageInfo, m_DepthImage, m_DepthImageAllocation);

        VkImageViewCreateInfo l_ViewInfo{};
        l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_ViewInfo.image = m_DepthImage;
        l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        l_ViewInfo.format = m_DepthFormat;
        l_ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        l_ViewInfo.subresourceRange.levelCount = 1;
        l_ViewInfo.subresourceRange.layerCount = 1;

        Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, m_HostAllocator, &m_DepthImageView), "VulkanGeometryPass: vkCreateImageView (depth) failed");
    }

    void VulkanGeometryPass::DestroyDepthImage()
    {
        if (m_DepthImageView != VK_NULL_HANDLE)
        {
            if (m_ResourceStateTracker)
            {
                m_ResourceStateTracker->ForgetImage(m_DepthImage);
            }

            vkDestroyImageView(m_Device, m_DepthImageView, m_HostAllocator);
            m_DepthImageView = VK_NULL_HANDLE;
        }

        if (m_DepthImage != VK_NULL_HANDLE)
        {
            m_Allocator->DestroyImage(m_DepthImage, m_DepthImageAllocation);
            m_DepthImage = VK_NULL_HANDLE;
            m_DepthImageAllocation = VK_NULL_HANDLE;
        }
    }

    void VulkanGeometryPass::CreatePipeline()
    {
        m_Shaders->Load("GeometryBuffer.vert", "Assets/Shaders/GeometryBuffer.vert.spv", ShaderStage::Vertex);
        m_Shaders->Load("GeometryBuffer.frag", "Assets/Shaders/GeometryBuffer.frag.spv", ShaderStage::Fragment);

        const std::vector<uint32_t>& l_VertSpirV = *m_Shaders->GetSpirV("GeometryBuffer.vert");
        const std::vector<uint32_t>& l_FragSpirV = *m_Shaders->GetSpirV("GeometryBuffer.frag");

        VkDescriptorSetLayoutBinding l_TexBinding{};
        l_TexBinding.binding = 0;
        l_TexBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_TexBinding.descriptorCount = 1;
        l_TexBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo l_SetLayoutInfo{};
        l_SetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        l_SetLayoutInfo.bindingCount = 1;
        l_SetLayoutInfo.pBindings = &l_TexBinding;
        Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device, &l_SetLayoutInfo, m_HostAllocator, &m_GBufferTextureSetLayout), "VulkanGeometryPass: vkCreateDescriptorSetLayout failed");

        VkPushConstantRange l_PC{};
        l_PC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        l_PC.offset = 0;
        l_PC.size = sizeof(GeometryBufferPushConstants);

        VkPipelineLayoutCreateInfo l_LayoutInfo{};
        l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_LayoutInfo.setLayoutCount = 1;
        l_LayoutInfo.pSetLayouts = &m_GBufferTextureSetLayout;
        l_LayoutInfo.pushConstantRangeCount = 1;
        l_LayoutInfo.pPushConstantRanges = &l_PC;
        Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device, &l_LayoutInfo, m_HostAllocator, &m_GBufferPipelineLayout), "VulkanGeometryPass: vkCreatePipelineLayout failed");

        auto a_CreateModule = [&](const std::vector<uint32_t>& spv) -> VkShaderModule
        {
            VkShaderModuleCreateInfo l_Info{};
            l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            l_Info.codeSize = spv.size() * 4;
            l_Info.pCode = spv.data();
            VkShaderModule l_Module = VK_NULL_HANDLE;
            Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device, &l_Info, m_HostAllocator, &l_Module), "VulkanGeometryPass: vkCreateShaderModule failed");

            return l_Module;
        };

        const VkShaderModule l_VertModule = a_CreateModule(l_VertSpirV);
        const VkShaderModule l_FragModule = a_CreateModule(l_FragSpirV);

        VkPipelineShaderStageCreateInfo l_Stages[2]{};
        l_Stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT,   l_VertModule, "main" };
        l_Stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, l_FragModule, "main" };

        VkVertexInputBindingDescription l_Binding{};
        l_Binding.binding = 0;
        l_Binding.stride = sizeof(Geometry::Vertex);
        l_Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription l_Attrs[3]{};
        l_Attrs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex, Position) };
        l_Attrs[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex, Normal) };
        l_Attrs[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Geometry::Vertex, UV) };

        VkPipelineVertexInputStateCreateInfo l_VertexInput{};
        l_VertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        l_VertexInput.vertexBindingDescriptionCount = 1;
        l_VertexInput.pVertexBindingDescriptions = &l_Binding;
        l_VertexInput.vertexAttributeDescriptionCount = 3;
        l_VertexInput.pVertexAttributeDescriptions = l_Attrs;

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
        l_DepthStencil.depthTestEnable = VK_TRUE;
        l_DepthStencil.depthWriteEnable = VK_TRUE;
        l_DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        VkPipelineColorBlendAttachmentState l_BlendAtt{};
        l_BlendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        const VkPipelineColorBlendAttachmentState l_BlendAtts[3] = { l_BlendAtt, l_BlendAtt, l_BlendAtt };
        VkPipelineColorBlendStateCreateInfo l_Blend{};
        l_Blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        l_Blend.attachmentCount = 3;
        l_Blend.pAttachments = l_BlendAtts;

        const VkDynamicState l_DynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo l_Dynamic{};
        l_Dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        l_Dynamic.dynamicStateCount = 2;
        l_Dynamic.pDynamicStates = l_DynStates;

        const VkFormat l_ColorFmts[3] =
        {
            TextureFormatToVkFormat(VulkanGeometryBuffer::AlbedoFormat),
            TextureFormatToVkFormat(VulkanGeometryBuffer::NormalFormat),
            TextureFormatToVkFormat(VulkanGeometryBuffer::MaterialFormat)
        };

        VkPipelineRenderingCreateInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        l_RenderingInfo.colorAttachmentCount = 3;
        l_RenderingInfo.pColorAttachmentFormats = l_ColorFmts;
        l_RenderingInfo.depthAttachmentFormat = m_DepthFormat;

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
        l_PipelineInfo.layout = m_GBufferPipelineLayout;

        Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &l_PipelineInfo, m_HostAllocator, &m_GBufferPipeline), "VulkanGeometryPass: vkCreateGraphicsPipelines failed");

        vkDestroyShaderModule(m_Device, l_VertModule, m_HostAllocator);
        vkDestroyShaderModule(m_Device, l_FragModule, m_HostAllocator);
    }

    void VulkanGeometryPass::DestroyPipeline()
    {
        if (m_GBufferPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_Device, m_GBufferPipeline, m_HostAllocator);
            m_GBufferPipeline = VK_NULL_HANDLE;
        }

        if (m_GBufferPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_Device, m_GBufferPipelineLayout, m_HostAllocator);
            m_GBufferPipelineLayout = VK_NULL_HANDLE;
        }

        if (m_GBufferTextureSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_Device, m_GBufferTextureSetLayout, m_HostAllocator);
            m_GBufferTextureSetLayout = VK_NULL_HANDLE;
        }
    }
}