#include "Trinity/Renderer/Vulkan/Passes/VulkanShadowPass.h"

#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/VulkanShaderInterop.h"
#include "Trinity/Geometry/Geometry.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>

namespace Trinity
{
    void VulkanShadowPass::Initialize(const VulkanPassContext& context)
    {
        TR_CORE_TRACE("Initializing VulkanShadowPass");

        CopyContext(context);

        CreateShadowMap();
        InitialShadowMapTransition();
        CreatePipeline();

        TR_CORE_TRACE("VulkanShadowPass Initialized");
    }

    void VulkanShadowPass::Shutdown()
    {
        TR_CORE_TRACE("Shutting Down VulkanShadowPass");

        DestroyPipeline();
        DestroyShadowMap();

        TR_CORE_TRACE("VulkanShadowPass Shutdown Complete");
    }

    void VulkanShadowPass::Recreate(uint32_t width, uint32_t height)
    {
        // Shadow map is fixed size — no work needed on viewport resize
        (void)width;
        (void)height;
    }

    void VulkanShadowPass::Reset()
    {
        m_DrawCommands.clear();
        m_HasShadowCaster = false;
    }

    void VulkanShadowPass::SetLightSpaceMatrix(const glm::mat4& matrix)
    {
        m_LightSpaceMatrix = matrix;
        m_HasShadowCaster = true;
    }

    void VulkanShadowPass::Submit(VertexBuffer& vb, IndexBuffer& ib, uint32_t indexCount, const glm::mat4& lightSpaceMVP)
    {
        ShadowDrawCommand l_Command{};
        l_Command.VB = &vb;
        l_Command.IB = &ib;
        l_Command.IndexCount = indexCount;
        l_Command.LightSpaceMVP = lightSpaceMVP;

        m_DrawCommands.push_back(l_Command);
    }

    void VulkanShadowPass::Execute(const VulkanFrameContext& frameContext)
    {
        if (!m_HasShadowCaster || m_DrawCommands.empty())
        {
            return;
        }

        const VkCommandBuffer l_CommandBuffer = frameContext.CommandBuffer;

        // Shadow map: DEPTH_STENCIL_READ_ONLY → DEPTH_STENCIL_ATTACHMENT
        {
            VkImageMemoryBarrier2 l_Barrier{};
            l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            l_Barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            l_Barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            l_Barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            l_Barrier.image = m_ShadowMapImage;
            l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

            VkDependencyInfo l_Dep{};
            l_Dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            l_Dep.imageMemoryBarrierCount = 1;
            l_Dep.pImageMemoryBarriers = &l_Barrier;
            vkCmdPipelineBarrier2(l_CommandBuffer, &l_Dep);
        }

        VkRenderingAttachmentInfo l_DepthAtt{};
        l_DepthAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        l_DepthAtt.imageView = m_ShadowMapView;
        l_DepthAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        l_DepthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        l_DepthAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        l_DepthAtt.clearValue.depthStencil = { 1.0f, 0 };

        VkRenderingInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        l_RenderingInfo.renderArea = { { 0, 0 }, { s_ShadowMapSize, s_ShadowMapSize } };
        l_RenderingInfo.layerCount = 1;
        l_RenderingInfo.pDepthAttachment = &l_DepthAtt;

        vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);

        VkViewport l_Viewport{ 0.0f, 0.0f, static_cast<float>(s_ShadowMapSize), static_cast<float>(s_ShadowMapSize), 0.0f, 1.0f };
        VkRect2D l_Scissor{ { 0, 0 }, { s_ShadowMapSize, s_ShadowMapSize } };
        vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);
        vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);
        vkCmdBindPipeline(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowPipeline);

        for (const ShadowDrawCommand& it_Command : m_DrawCommands)
        {
            VkBuffer l_VkVB = reinterpret_cast<VkBuffer>(it_Command.VB->GetNativeHandle());
            VkBuffer l_VkIB = reinterpret_cast<VkBuffer>(it_Command.IB->GetNativeHandle());
            VkDeviceSize l_Offset = 0;

            vkCmdBindVertexBuffers(l_CommandBuffer, 0, 1, &l_VkVB, &l_Offset);
            vkCmdBindIndexBuffer(l_CommandBuffer, l_VkIB, 0, (it_Command.IB->GetIndexType() == IndexType::UInt16) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

            ShadowPushConstants l_PC{};
            l_PC.LightSpaceModelViewProjection = it_Command.LightSpaceMVP;
            vkCmdPushConstants(l_CommandBuffer, m_ShadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowPushConstants), &l_PC);

            vkCmdDrawIndexed(l_CommandBuffer, it_Command.IndexCount, 1, 0, 0, 0);
        }

        vkCmdEndRendering(l_CommandBuffer);

        // Shadow map: DEPTH_STENCIL_ATTACHMENT → DEPTH_STENCIL_READ_ONLY
        {
            VkImageMemoryBarrier2 l_Barrier{};
            l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            l_Barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            l_Barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            l_Barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            l_Barrier.image = m_ShadowMapImage;
            l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

            VkDependencyInfo l_Dep{};
            l_Dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            l_Dep.imageMemoryBarrierCount = 1;
            l_Dep.pImageMemoryBarriers = &l_Barrier;
            vkCmdPipelineBarrier2(l_CommandBuffer, &l_Dep);
        }
    }

    // -----------------------------------------------------------------

    void VulkanShadowPass::CreateShadowMap()
    {
        VkImageCreateInfo l_ImageInfo{};
        l_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        l_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
        l_ImageInfo.extent = { s_ShadowMapSize, s_ShadowMapSize, 1 };
        l_ImageInfo.mipLevels = 1;
        l_ImageInfo.arrayLayers = 1;
        l_ImageInfo.format = m_DepthFormat;
        l_ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        l_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        l_ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        l_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        l_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        m_Allocator->CreateImage(l_ImageInfo, m_ShadowMapImage, m_ShadowMapAllocation);

        VkImageViewCreateInfo l_ViewInfo{};
        l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_ViewInfo.image = m_ShadowMapImage;
        l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        l_ViewInfo.format = m_DepthFormat;
        l_ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        l_ViewInfo.subresourceRange.levelCount = 1;
        l_ViewInfo.subresourceRange.layerCount = 1;

        Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, m_HostAllocator, &m_ShadowMapView), "VulkanShadowPass: vkCreateImageView failed");

        VkSamplerCreateInfo l_SamplerInfo{};
        l_SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        l_SamplerInfo.magFilter = VK_FILTER_LINEAR;
        l_SamplerInfo.minFilter = VK_FILTER_LINEAR;
        l_SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        l_SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        l_SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        l_SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        l_SamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        l_SamplerInfo.compareEnable = VK_TRUE;
        l_SamplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        l_SamplerInfo.maxLod = 0.0f;
        l_SamplerInfo.maxAnisotropy = 1.0f;

        Utilities::VulkanUtilities::VKCheck(vkCreateSampler(m_Device, &l_SamplerInfo, m_HostAllocator, &m_ShadowMapSampler), "VulkanShadowPass: vkCreateSampler failed");
    }

    void VulkanShadowPass::InitialShadowMapTransition()
    {
        // Transition from UNDEFINED to DEPTH_STENCIL_READ_ONLY so the lighting pass
        // can sample it even on frames where no shadows are cast.
        VkCommandPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        l_PoolInfo.queueFamilyIndex = m_GraphicsQueueFamilyIndex;

        VkCommandPool l_Pool = VK_NULL_HANDLE;
        Utilities::VulkanUtilities::VKCheck(vkCreateCommandPool(m_Device, &l_PoolInfo, m_HostAllocator, &l_Pool), "VulkanShadowPass: vkCreateCommandPool failed");

        VkCommandBufferAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocInfo.commandPool = l_Pool;
        l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocInfo.commandBufferCount = 1;

        VkCommandBuffer l_CommandBuffer = VK_NULL_HANDLE;
        Utilities::VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_Device, &l_AllocInfo, &l_CommandBuffer), "VulkanShadowPass: vkAllocateCommandBuffers failed");

        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        Utilities::VulkanUtilities::VKCheck(vkBeginCommandBuffer(l_CommandBuffer, &l_BeginInfo), "VulkanShadowPass: vkBeginCommandBuffer failed");

        VkImageMemoryBarrier2 l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        l_Barrier.srcAccessMask = VK_ACCESS_2_NONE;
        l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
        l_Barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        l_Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        l_Barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        l_Barrier.image = m_ShadowMapImage;
        l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

        VkDependencyInfo l_Dep{};
        l_Dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_Dep.imageMemoryBarrierCount = 1;
        l_Dep.pImageMemoryBarriers = &l_Barrier;
        vkCmdPipelineBarrier2(l_CommandBuffer, &l_Dep);

        Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(l_CommandBuffer), "VulkanShadowPass: vkEndCommandBuffer failed");

        VkSubmitInfo l_Submit{};
        l_Submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_Submit.commandBufferCount = 1;
        l_Submit.pCommandBuffers = &l_CommandBuffer;

        Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_GraphicsQueue, 1, &l_Submit, VK_NULL_HANDLE), "VulkanShadowPass: vkQueueSubmit failed");
        Utilities::VulkanUtilities::VKCheck(vkQueueWaitIdle(m_GraphicsQueue), "VulkanShadowPass: vkQueueWaitIdle failed");

        vkDestroyCommandPool(m_Device, l_Pool, m_HostAllocator);
    }

    void VulkanShadowPass::CreatePipeline()
    {
        m_Shaders->Load("Shadow.vert", "Assets/Shaders/Shadow.vert.spv", ShaderStage::Vertex);
        m_Shaders->Load("Shadow.frag", "Assets/Shaders/Shadow.frag.spv", ShaderStage::Fragment);

        const std::vector<uint32_t>& l_VertSpirV = *m_Shaders->GetSpirV("Shadow.vert");
        const std::vector<uint32_t>& l_FragSpirV = *m_Shaders->GetSpirV("Shadow.frag");

        // Empty descriptor set layout (shadow shader has no descriptors)
        VkDescriptorSetLayoutCreateInfo l_EmptyLayout{};
        l_EmptyLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device, &l_EmptyLayout, m_HostAllocator, &m_ShadowSetLayout), "VulkanShadowPass: vkCreateDescriptorSetLayout failed");

        VkPushConstantRange l_PC{};
        l_PC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        l_PC.offset = 0;
        l_PC.size = sizeof(ShadowPushConstants);

        VkPipelineLayoutCreateInfo l_LayoutInfo{};
        l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_LayoutInfo.setLayoutCount = 1;
        l_LayoutInfo.pSetLayouts = &m_ShadowSetLayout;
        l_LayoutInfo.pushConstantRangeCount = 1;
        l_LayoutInfo.pPushConstantRanges = &l_PC;
        Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device, &l_LayoutInfo, m_HostAllocator, &m_ShadowPipelineLayout), "VulkanShadowPass: vkCreatePipelineLayout failed");

        auto a_CreateModule = [&](const std::vector<uint32_t>& spv) -> VkShaderModule
        {
            VkShaderModuleCreateInfo l_Info{};
            l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            l_Info.codeSize = spv.size() * 4;
            l_Info.pCode = spv.data();
            VkShaderModule l_Module = VK_NULL_HANDLE;
            Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device, &l_Info, m_HostAllocator, &l_Module), "VulkanShadowPass: vkCreateShaderModule failed");
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
        l_Raster.cullMode = VK_CULL_MODE_FRONT_BIT;
        l_Raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        l_Raster.lineWidth = 1.0f;
        l_Raster.depthBiasEnable = VK_TRUE;
        l_Raster.depthBiasConstantFactor = 2.0f;
        l_Raster.depthBiasSlopeFactor = 2.5f;

        VkPipelineMultisampleStateCreateInfo l_MSAA{};
        l_MSAA.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        l_MSAA.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo l_DepthStencil{};
        l_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        l_DepthStencil.depthTestEnable = VK_TRUE;
        l_DepthStencil.depthWriteEnable = VK_TRUE;
        l_DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        VkPipelineColorBlendStateCreateInfo l_Blend{};
        l_Blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

        const VkDynamicState l_DynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo l_Dynamic{};
        l_Dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        l_Dynamic.dynamicStateCount = 2;
        l_Dynamic.pDynamicStates = l_DynStates;

        VkPipelineRenderingCreateInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        l_RenderingInfo.depthAttachmentFormat = m_DepthFormat;
        l_RenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

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
        l_PipelineInfo.layout = m_ShadowPipelineLayout;

        Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &l_PipelineInfo, m_HostAllocator, &m_ShadowPipeline), "VulkanShadowPass: vkCreateGraphicsPipelines failed");

        vkDestroyShaderModule(m_Device, l_VertModule, m_HostAllocator);
        vkDestroyShaderModule(m_Device, l_FragModule, m_HostAllocator);
    }

    void VulkanShadowPass::DestroyShadowMap()
    {
        if (m_ShadowMapSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_Device, m_ShadowMapSampler, m_HostAllocator);
            m_ShadowMapSampler = VK_NULL_HANDLE;
        }

        if (m_ShadowMapView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, m_ShadowMapView, m_HostAllocator);
            m_ShadowMapView = VK_NULL_HANDLE;
        }

        if (m_ShadowMapImage != VK_NULL_HANDLE)
        {
            m_Allocator->DestroyImage(m_ShadowMapImage, m_ShadowMapAllocation);
            m_ShadowMapImage = VK_NULL_HANDLE;
            m_ShadowMapAllocation = VK_NULL_HANDLE;
        }
    }

    void VulkanShadowPass::DestroyPipeline()
    {
        if (m_ShadowPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_Device, m_ShadowPipeline, m_HostAllocator);
            m_ShadowPipeline = VK_NULL_HANDLE;
        }

        if (m_ShadowPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_Device, m_ShadowPipelineLayout, m_HostAllocator);
            m_ShadowPipelineLayout = VK_NULL_HANDLE;
        }

        if (m_ShadowSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_Device, m_ShadowSetLayout, m_HostAllocator);
            m_ShadowSetLayout = VK_NULL_HANDLE;
        }
    }
}