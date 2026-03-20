#include "Trinity/Renderer/Vulkan/Passes/VulkanLightingPass.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <array>
#include <cstdlib>

namespace Trinity
{
    void VulkanLightingPass::Initialize(const VulkanPassContext& context)
    {
        TR_CORE_TRACE("Initializing VulkanLightingPass");

        CopyContext(context);

        // GBuffer sampler — linear, clamp, no compare
        {
            VkSamplerCreateInfo l_Info{};
            l_Info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            l_Info.magFilter = VK_FILTER_LINEAR;
            l_Info.minFilter = VK_FILTER_LINEAR;
            l_Info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            l_Info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            l_Info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            l_Info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            l_Info.maxLod = 0.0f;
            l_Info.maxAnisotropy = 1.0f;
            Utilities::VulkanUtilities::VKCheck(vkCreateSampler(m_Device, &l_Info, m_HostAllocator, &m_GBufferSampler), "VulkanLightingPass: vkCreateSampler (GBuffer) failed");
        }

        // Per-frame UBOs
        m_LightingUBOs.reserve(m_FramesInFlight);
        for (uint32_t i = 0; i < m_FramesInFlight; ++i)
        {
            m_LightingUBOs.emplace_back(*m_Allocator, sizeof(LightingUniformData));
        }

        // Per-frame descriptor pools (2 sets per frame: GBuffer set + UBO set)
        const std::array<VkDescriptorPoolSize, 2> l_Sizes =
        { {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 * m_FramesInFlight },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * m_FramesInFlight }
        } };

        m_LightingDescriptorPools.resize(m_FramesInFlight, VK_NULL_HANDLE);
        for (uint32_t i = 0; i < m_FramesInFlight; ++i)
        {
            VkDescriptorPoolCreateInfo l_PoolInfo{};
            l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            l_PoolInfo.maxSets = 2;
            l_PoolInfo.poolSizeCount = static_cast<uint32_t>(l_Sizes.size());
            l_PoolInfo.pPoolSizes = l_Sizes.data();
            Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Device, &l_PoolInfo, m_HostAllocator, &m_LightingDescriptorPools[i]), "VulkanLightingPass: vkCreateDescriptorPool failed");
        }

        CreatePipeline();

        // Scene viewport image is created on first Recreate call with valid dimensions.

        TR_CORE_TRACE("VulkanLightingPass Initialized");
    }

    void VulkanLightingPass::Shutdown()
    {
        TR_CORE_TRACE("Shutting Down VulkanLightingPass");

        DestroySceneViewportImage();
        DestroyPipeline();

        m_LightingUBOs.clear();

        for (VkDescriptorPool& it_Pool : m_LightingDescriptorPools)
        {
            if (it_Pool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(m_Device, it_Pool, m_HostAllocator);
                it_Pool = VK_NULL_HANDLE;
            }
        }
        m_LightingDescriptorPools.clear();

        if (m_GBufferSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_Device, m_GBufferSampler, m_HostAllocator);
            m_GBufferSampler = VK_NULL_HANDLE;
        }

        TR_CORE_TRACE("VulkanLightingPass Shutdown Complete");
    }

    void VulkanLightingPass::Recreate(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        DestroySceneViewportImage();
        CreateSceneViewportImage(width, height);

        TR_CORE_TRACE("VulkanLightingPass Recreated ({}x{})", width, height);
    }

    void VulkanLightingPass::SetGBufferResources(VkImageView albedo, VkImageView normal, VkImageView material, VkImageView depth)
    {
        m_AlbedoView = albedo;
        m_NormalView = normal;
        m_MaterialView = material;
        m_DepthView = depth;
    }

    void VulkanLightingPass::SetShadowResources(VkImageView shadowMapView, VkSampler shadowMapSampler)
    {
        m_ShadowMapView = shadowMapView;
        m_ShadowMapSampler = shadowMapSampler;
    }

    void VulkanLightingPass::Submit(const LightingSubmitData& data)
    {
        m_SubmitData = data;
        m_HasSubmitData = true;
    }

    void VulkanLightingPass::Execute(const VulkanFrameContext& frameContext)
    {
        if (!m_HasSubmitData || m_SceneViewportImage == VK_NULL_HANDLE)
        {
            m_HasSubmitData = false;
            return;
        }

        const VkCommandBuffer l_CB = frameContext.CommandBuffer;
        const uint32_t l_FrameIndex = frameContext.FrameIndex;
        const uint32_t l_Width = frameContext.ViewportWidth;
        const uint32_t l_Height = frameContext.ViewportHeight;

        // Reset descriptor pool for this frame
        vkResetDescriptorPool(m_Device, m_LightingDescriptorPools[l_FrameIndex], 0);

        // Upload light data
        m_LightingUBOs[l_FrameIndex].SetData(&m_SubmitData.UniformData, sizeof(LightingUniformData));

        // Transition scene viewport image to COLOR_ATTACHMENT
        const VkImageSubresourceRange l_ColorRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        m_ResourceStateTracker->TransitionImageResource(l_CB, m_SceneViewportImage, l_ColorRange, g_ColorAttachmentWriteImageState);

        VkClearValue l_Clear{};
        VkRenderingAttachmentInfo l_ColorAtt{};
        l_ColorAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        l_ColorAtt.imageView = m_SceneViewportImageView;
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
        VkRect2D l_Scissor{ { 0, 0 }, { l_Width, l_Height } };
        vkCmdSetViewport(l_CB, 0, 1, &l_Viewport);
        vkCmdSetScissor(l_CB, 0, 1, &l_Scissor);
        vkCmdBindPipeline(l_CB, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipeline);

        // Build GBuffer descriptor set (set 0)
        {
            VkDescriptorSetAllocateInfo l_AllocInfo{};
            l_AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            l_AllocInfo.descriptorPool = m_LightingDescriptorPools[l_FrameIndex];
            l_AllocInfo.descriptorSetCount = 1;
            l_AllocInfo.pSetLayouts = &m_LightingGBufferSetLayout;

            VkDescriptorSet l_GBufSet = VK_NULL_HANDLE;
            Utilities::VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Device, &l_AllocInfo, &l_GBufSet), "VulkanLightingPass: vkAllocateDescriptorSets (GBuffer set) failed");

            const VkImageLayout l_SRO = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            const VkImageLayout l_DSR = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

            VkDescriptorImageInfo l_ImageInfos[5]{};
            l_ImageInfos[0] = { m_GBufferSampler, m_AlbedoView,    l_SRO };
            l_ImageInfos[1] = { m_GBufferSampler, m_NormalView,    l_SRO };
            l_ImageInfos[2] = { m_GBufferSampler, m_MaterialView,  l_SRO };
            l_ImageInfos[3] = { m_GBufferSampler, m_DepthView,     l_DSR };
            l_ImageInfos[4] = { m_ShadowMapSampler, m_ShadowMapView, l_DSR };

            VkWriteDescriptorSet l_Writes[5]{};
            for (uint32_t i = 0; i < 5; ++i)
            {
                l_Writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                l_Writes[i].dstSet = l_GBufSet;
                l_Writes[i].dstBinding = i;
                l_Writes[i].descriptorCount = 1;
                l_Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                l_Writes[i].pImageInfo = &l_ImageInfos[i];
            }
            vkUpdateDescriptorSets(m_Device, 5, l_Writes, 0, nullptr);

            // Build UBO descriptor set (set 1)
            VkDescriptorSetAllocateInfo l_UBOAllocInfo{};
            l_UBOAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            l_UBOAllocInfo.descriptorPool = m_LightingDescriptorPools[l_FrameIndex];
            l_UBOAllocInfo.descriptorSetCount = 1;
            l_UBOAllocInfo.pSetLayouts = &m_LightingUBOSetLayout;

            VkDescriptorSet l_UBOSet = VK_NULL_HANDLE;
            Utilities::VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Device, &l_UBOAllocInfo, &l_UBOSet), "VulkanLightingPass: vkAllocateDescriptorSets (UBO set) failed");

            const VkDescriptorBufferInfo l_BufInfo = m_LightingUBOs[l_FrameIndex].GetDescriptorBufferInfo();
            VkWriteDescriptorSet l_UBOWrite{};
            l_UBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            l_UBOWrite.dstSet = l_UBOSet;
            l_UBOWrite.dstBinding = 0;
            l_UBOWrite.descriptorCount = 1;
            l_UBOWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            l_UBOWrite.pBufferInfo = &l_BufInfo;
            vkUpdateDescriptorSets(m_Device, 1, &l_UBOWrite, 0, nullptr);

            const VkDescriptorSet l_Sets[2] = { l_GBufSet, l_UBOSet };
            vkCmdBindDescriptorSets(l_CB, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipelineLayout, 0, 2, l_Sets, 0, nullptr);
        }

        LightingPushConstants l_PC{};
        l_PC.InvViewProjection = m_SubmitData.InvViewProjection;
        l_PC.CameraPosition = glm::vec4(m_SubmitData.CameraPosition, 1.0f);
        l_PC.ColorOutputTransfer = static_cast<uint32_t>(m_ColorPolicy.SceneOutputTransfer);
        l_PC.CameraNear = m_SubmitData.CameraNear;
        l_PC.CameraFar = m_SubmitData.CameraFar;
        l_PC.Exposure = 1.0f;
        vkCmdPushConstants(l_CB, m_LightingPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightingPushConstants), &l_PC);

        vkCmdDraw(l_CB, 3, 1, 0, 0);
        vkCmdEndRendering(l_CB);

        // Transition to SHADER_READ_ONLY for the post-process pass
        m_ResourceStateTracker->TransitionImageResource(l_CB, m_SceneViewportImage, l_ColorRange, g_ShaderReadOnlyImageState);

        m_HasSubmitData = false;
    }

    // -----------------------------------------------------------------

    void VulkanLightingPass::CreateSceneViewportImage(uint32_t width, uint32_t height)
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

        m_Allocator->CreateImage(l_ImageInfo, m_SceneViewportImage, m_SceneViewportImageAllocation);

        VkImageViewCreateInfo l_ViewInfo{};
        l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_ViewInfo.image = m_SceneViewportImage;
        l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        l_ViewInfo.format = m_SwapchainFormat;
        l_ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        l_ViewInfo.subresourceRange.levelCount = 1;
        l_ViewInfo.subresourceRange.layerCount = 1;
        Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, m_HostAllocator, &m_SceneViewportImageView), "VulkanLightingPass: vkCreateImageView failed");

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
        Utilities::VulkanUtilities::VKCheck(vkCreateSampler(m_Device, &l_SamplerInfo, m_HostAllocator, &m_SceneViewportSampler), "VulkanLightingPass: vkCreateSampler failed");
    }

    void VulkanLightingPass::DestroySceneViewportImage()
    {
        if (m_SceneViewportSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_Device, m_SceneViewportSampler, m_HostAllocator);
            m_SceneViewportSampler = VK_NULL_HANDLE;
        }

        if (m_SceneViewportImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, m_SceneViewportImageView, m_HostAllocator);
            m_SceneViewportImageView = VK_NULL_HANDLE;
        }

        if (m_SceneViewportImage != VK_NULL_HANDLE)
        {
            if (m_ResourceStateTracker)
            {
                m_ResourceStateTracker->ForgetImage(m_SceneViewportImage);
            }
            m_Allocator->DestroyImage(m_SceneViewportImage, m_SceneViewportImageAllocation);
            m_SceneViewportImage = VK_NULL_HANDLE;
            m_SceneViewportImageAllocation = VK_NULL_HANDLE;
        }
    }

    void VulkanLightingPass::CreatePipeline()
    {
        m_Shaders->Load("Lighting.vert", "Assets/Shaders/Lighting.vert.spv", ShaderStage::Vertex);
        m_Shaders->Load("Lighting.frag", "Assets/Shaders/Lighting.frag.spv", ShaderStage::Fragment);

        const std::vector<uint32_t>& l_VertSpirV = *m_Shaders->GetSpirV("Lighting.vert");
        const std::vector<uint32_t>& l_FragSpirV = *m_Shaders->GetSpirV("Lighting.frag");

        // Set 0: 5 combined image samplers (albedo, normal, material, depth, shadowmap)
        const VkDescriptorType l_CIS = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        VkDescriptorSetLayoutBinding l_GBufBindings[5]{};
        for (uint32_t i = 0; i < 5; ++i)
        {
            l_GBufBindings[i] = { i, l_CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
        }

        VkDescriptorSetLayoutCreateInfo l_GBufSetInfo{};
        l_GBufSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        l_GBufSetInfo.bindingCount = 5;
        l_GBufSetInfo.pBindings = l_GBufBindings;
        Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device, &l_GBufSetInfo, m_HostAllocator, &m_LightingGBufferSetLayout), "VulkanLightingPass: vkCreateDescriptorSetLayout (GBuffer set) failed");

        // Set 1: UBO
        VkDescriptorSetLayoutBinding l_UBOBinding{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
        VkDescriptorSetLayoutCreateInfo l_UBOSetInfo{};
        l_UBOSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        l_UBOSetInfo.bindingCount = 1;
        l_UBOSetInfo.pBindings = &l_UBOBinding;
        Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device, &l_UBOSetInfo, m_HostAllocator, &m_LightingUBOSetLayout), "VulkanLightingPass: vkCreateDescriptorSetLayout (UBO set) failed");

        VkPushConstantRange l_PC{};
        l_PC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        l_PC.offset = 0;
        l_PC.size = sizeof(LightingPushConstants);

        const VkDescriptorSetLayout l_Layouts[2] = { m_LightingGBufferSetLayout, m_LightingUBOSetLayout };
        VkPipelineLayoutCreateInfo l_LayoutInfo{};
        l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_LayoutInfo.setLayoutCount = 2;
        l_LayoutInfo.pSetLayouts = l_Layouts;
        l_LayoutInfo.pushConstantRangeCount = 1;
        l_LayoutInfo.pPushConstantRanges = &l_PC;
        Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device, &l_LayoutInfo, m_HostAllocator, &m_LightingPipelineLayout), "VulkanLightingPass: vkCreatePipelineLayout failed");

        auto a_CreateModule = [&](const std::vector<uint32_t>& spv) -> VkShaderModule
        {
            VkShaderModuleCreateInfo l_Info{};
            l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            l_Info.codeSize = spv.size() * 4;
            l_Info.pCode = spv.data();
            VkShaderModule l_Module = VK_NULL_HANDLE;
            Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device, &l_Info, m_HostAllocator, &l_Module), "VulkanLightingPass: vkCreateShaderModule failed");
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
        l_PipelineInfo.layout = m_LightingPipelineLayout;

        Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &l_PipelineInfo, m_HostAllocator, &m_LightingPipeline), "VulkanLightingPass: vkCreateGraphicsPipelines failed");

        vkDestroyShaderModule(m_Device, l_VertModule, m_HostAllocator);
        vkDestroyShaderModule(m_Device, l_FragModule, m_HostAllocator);
    }

    void VulkanLightingPass::DestroyPipeline()
    {
        if (m_LightingPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_Device, m_LightingPipeline, m_HostAllocator);
            m_LightingPipeline = VK_NULL_HANDLE;
        }

        if (m_LightingPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_Device, m_LightingPipelineLayout, m_HostAllocator);
            m_LightingPipelineLayout = VK_NULL_HANDLE;
        }

        if (m_LightingGBufferSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_Device, m_LightingGBufferSetLayout, m_HostAllocator);
            m_LightingGBufferSetLayout = VK_NULL_HANDLE;
        }

        if (m_LightingUBOSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_Device, m_LightingUBOSetLayout, m_HostAllocator);
            m_LightingUBOSetLayout = VK_NULL_HANDLE;
        }
    }
}