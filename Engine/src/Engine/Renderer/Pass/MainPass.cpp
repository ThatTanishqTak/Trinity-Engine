#include "Engine/Renderer/Pass/MainPass.h"
#include "Engine/Renderer/Pass/DrawPushConstants.h"

#include "Engine/Renderer/Vulkan/VulkanDevice.h"
#include "Engine/Renderer/Vulkan/VulkanDescriptors.h"
#include "Engine/Renderer/Vulkan/VulkanFrameResources.h"
#include "Engine/Renderer/Vulkan/VulkanRenderer.h"
#include "Engine/Renderer/Vulkan/VulkanSwapchain.h"
#include "Engine/Utilities/Utilities.h"

#include <array>
#include <functional>
#include <cstring>

#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
    // Shader paths for the main pass pipeline.
    static const char* s_DefaultVertShader = "Assets/Shaders/Simple.vert.spv";
    static const char* s_DefaultFragShader = "Assets/Shaders/Simple.frag.spv";

    void MainPass::SetClearColor(const glm::vec4& clearColor)
    {
        m_ClearColor = clearColor;
    }

    void MainPass::Clear()
    {
        m_ClearRequested = true;
    }

    void MainPass::DrawCube(const glm::vec3& size, const glm::vec3& position, const glm::vec4& tint)
    {
        m_PendingCubes.push_back({ size, position, tint });
    }

    void MainPass::Initialize(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources)
    {
        TR_CORE_INFO("MainPass initialize");

        m_Device = &device;
        m_FrameResources = &frameResources;
        m_Extent = swapchain.GetExtent();

        CreateRenderPass(device, swapchain);
        CreateFramebuffers(device, swapchain);
        CreateGlobalUniformBuffers(device, frameResources);
        CreatePipeline(device, swapchain, frameResources);
    }

    void MainPass::OnResize(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources, VulkanRenderer& renderer)
    {
        TR_CORE_INFO("MainPass resize");

        // Pipeline depends on the render pass, so shut it down before destroying swapchain resources.
        const auto l_SubmitResourceFree = [&renderer](std::function<void()>&& function)
        {
            renderer.SubmitResourceFree(std::move(function));
        };

        m_Pipeline.Shutdown(device, l_SubmitResourceFree);

        // Swapchain-dependent resources must be destroyed before rebuilding them for the new extent.
        DestroySwapchainResources(device, renderer);

        if (m_FrameResources && m_FrameResources->GetFramesInFlight() != frameResources.GetFramesInFlight())
        {
            DestroyGlobalUniformBuffers(device);
        }

        m_Device = &device;
        m_FrameResources = &frameResources;
        m_Extent = swapchain.GetExtent();

        CreateRenderPass(device, swapchain);
        CreateFramebuffers(device, swapchain);
        CreateGlobalUniformBuffers(device, frameResources);
        CreatePipeline(device, swapchain, frameResources);
    }

    void MainPass::RecordCommandBuffer(VkCommandBuffer command, uint32_t imageIndex, uint32_t currentFrame, VulkanRenderer& renderer)
    {
        VkCommandBufferBeginInfo l_CommandBufferBeginInfo{};
        l_CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        Utilities::VulkanUtilities::VKCheckStrict(vkBeginCommandBuffer(command, &l_CommandBufferBeginInfo), "vkBeginCommandBuffer");

        VkClearValue l_ClearValue{};
        l_ClearValue.color.float32[0] = m_ClearColor.r;
        l_ClearValue.color.float32[1] = m_ClearColor.g;
        l_ClearValue.color.float32[2] = m_ClearColor.b;
        l_ClearValue.color.float32[3] = m_ClearColor.a;

        VkRenderPassBeginInfo l_RenderPassBeginInfo{};
        l_RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        l_RenderPassBeginInfo.renderPass = m_RenderPass;
        l_RenderPassBeginInfo.framebuffer = m_Framebuffers[imageIndex];
        l_RenderPassBeginInfo.renderArea.offset = { 0, 0 };
        l_RenderPassBeginInfo.renderArea.extent = m_Extent;
        l_RenderPassBeginInfo.clearValueCount = 1;
        l_RenderPassBeginInfo.pClearValues = &l_ClearValue;

        vkCmdBeginRenderPass(command, &l_RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (m_Pipeline.IsValid())
        {
            VkViewport l_Viewport{};
            l_Viewport.x = 0.0f;
            l_Viewport.y = 0.0f;
            l_Viewport.width = (float)m_Extent.width;
            l_Viewport.height = (float)m_Extent.height;
            l_Viewport.minDepth = 0.0f;
            l_Viewport.maxDepth = 1.0f;

            VkRect2D l_Scissor{};
            l_Scissor.offset = { 0, 0 };
            l_Scissor.extent = m_Extent;

            vkCmdSetViewport(command, 0, 1, &l_Viewport);
            vkCmdSetScissor(command, 0, 1, &l_Scissor);

            vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.GetPipeline());

            // Bind per-frame descriptor set at set 0 to match the pipeline layout.
            VkDescriptorSet l_DescriptorSet = VK_NULL_HANDLE;
            if (m_Device && m_FrameResources && m_FrameResources->HasDescriptors() && currentFrame < m_GlobalUniformBuffers.size())
            {
                VulkanResources::BufferResource& l_GlobalBuffer = m_GlobalUniformBuffers[currentFrame];
                l_DescriptorSet = m_FrameResources->AllocateGlobalSet(currentFrame);
                if (l_DescriptorSet != VK_NULL_HANDLE && l_GlobalBuffer.Buffer != VK_NULL_HANDLE && l_GlobalBuffer.Memory != VK_NULL_HANDLE)
                {
                    GlobalUniformData l_GlobalData{};
                    void* l_MappedData = nullptr;
                    Utilities::VulkanUtilities::VKCheckStrict(vkMapMemory(m_Device->GetDevice(), l_GlobalBuffer.Memory, 0, sizeof(GlobalUniformData), 0, &l_MappedData),
                        "vkMapMemory(GlobalUniformData)");
                    if (l_MappedData)
                    {
                        std::memcpy(l_MappedData, &l_GlobalData, sizeof(GlobalUniformData));
                    }
                    vkUnmapMemory(m_Device->GetDevice(), l_GlobalBuffer.Memory);

                    VkDescriptorBufferInfo l_BufferInfo{};
                    l_BufferInfo.buffer = l_GlobalBuffer.Buffer;
                    l_BufferInfo.offset = 0;
                    l_BufferInfo.range = sizeof(GlobalUniformData);

                    const VulkanDescriptors& l_Descriptors = m_FrameResources->GetDescriptors();
                    l_Descriptors.WriteBuffer(l_DescriptorSet, 0, l_BufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

                    VkDescriptorBufferInfo l_TransformBufferInfo{};
                    l_TransformBufferInfo.buffer = renderer.GetTransformBufferForFrame();
                    l_TransformBufferInfo.offset = 0;
                    l_TransformBufferInfo.range = VK_WHOLE_SIZE;

                    if (l_TransformBufferInfo.buffer != VK_NULL_HANDLE)
                    {
                        l_Descriptors.WriteBuffer(l_DescriptorSet, 1, l_TransformBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
                    }

                    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.GetPipelineLayout(), 0, 1, &l_DescriptorSet, 0, nullptr);
                }
            }

            // The shaders must be written to use gl_VertexIndex (no vertex buffers).
            // If no draw calls were queued, keep the default triangle so you see something.
            if (m_PendingCubes.empty())
            {
                glm::mat4 l_TransformMatrix{ 1.0f };
                const uint32_t l_TransformIndex = renderer.PushTransform(l_TransformMatrix);

                DrawPushConstant l_DrawPushConstant{};
                l_DrawPushConstant.TransformIndex = l_TransformIndex;
                vkCmdPushConstants(command, m_Pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DrawPushConstant), &l_DrawPushConstant);
                vkCmdDraw(command, 3, 1, 0, 0);
            }
            else
            {
                // Placeholder draw loop for cube requests.
                for (const auto& it_Cube : m_PendingCubes)
                {
                    glm::mat4 l_TransformMatrix{ 1.0f };
                    l_TransformMatrix = glm::translate(l_TransformMatrix, it_Cube.m_Position);
                    l_TransformMatrix = glm::scale(l_TransformMatrix, it_Cube.m_Size);

                    const uint32_t l_TransformIndex = renderer.PushTransform(l_TransformMatrix);

                    DrawPushConstant l_DrawPushConstant{};
                    l_DrawPushConstant.TransformIndex = l_TransformIndex;
                    vkCmdPushConstants(command, m_Pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DrawPushConstant), &l_DrawPushConstant);
                    vkCmdDraw(command, 36, 1, 0, 0);
                }
            }
        }

        vkCmdEndRenderPass(command);

        Utilities::VulkanUtilities::VKCheckStrict(vkEndCommandBuffer(command), "vkEndCommandBuffer");

        m_PendingCubes.clear();
        m_ClearRequested = false;
    }

    void MainPass::OnDestroy(VulkanDevice& device)
    {
        m_Pipeline.Shutdown(device);
        DestroyGlobalUniformBuffers(device);
        DestroySwapchainResources(device);

        m_FrameResources = nullptr;
        m_Device = nullptr;

        TR_CORE_INFO("MainPass destroy");
    }

    void MainPass::CreateRenderPass(VulkanDevice& device, VulkanSwapchain& swapchain)
    {
        if (m_RenderPass)
        {
            vkDestroyRenderPass(device.GetDevice(), m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }

        VkAttachmentDescription l_ColorAttachmentDescription{};
        l_ColorAttachmentDescription.format = swapchain.GetImageFormat();
        l_ColorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        l_ColorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        l_ColorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        l_ColorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        l_ColorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        l_ColorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        l_ColorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference l_ColorAttachmentReference{};
        l_ColorAttachmentReference.attachment = 0;
        l_ColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription l_SubpassDescription{};
        l_SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        l_SubpassDescription.colorAttachmentCount = 1;
        l_SubpassDescription.pColorAttachments = &l_ColorAttachmentReference;

        VkSubpassDependency l_SubpassDependency{};
        l_SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        l_SubpassDependency.dstSubpass = 0;
        l_SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        l_SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        l_SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo l_RenderPassCreateInfo{};
        l_RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        l_RenderPassCreateInfo.attachmentCount = 1;
        l_RenderPassCreateInfo.pAttachments = &l_ColorAttachmentDescription;
        l_RenderPassCreateInfo.subpassCount = 1;
        l_RenderPassCreateInfo.pSubpasses = &l_SubpassDescription;
        l_RenderPassCreateInfo.dependencyCount = 1;
        l_RenderPassCreateInfo.pDependencies = &l_SubpassDependency;

        TR_CORE_INFO("MainPass create render pass");
        Utilities::VulkanUtilities::VKCheckStrict(vkCreateRenderPass(device.GetDevice(), &l_RenderPassCreateInfo, nullptr, &m_RenderPass), "vkCreateRenderPass");
    }

    void MainPass::CreateFramebuffers(VulkanDevice& device, VulkanSwapchain& swapchain)
    {
        for (auto it_Framebuffer : m_Framebuffers)
        {
            if (it_Framebuffer)
            {
                vkDestroyFramebuffer(device.GetDevice(), it_Framebuffer, nullptr);
            }
        }
        m_Framebuffers.clear();

        const auto& a_ImageViews = swapchain.GetImageViews();
        m_Framebuffers.resize(a_ImageViews.size());

        for (size_t i = 0; i < a_ImageViews.size(); ++i)
        {
            VkImageView l_ImageViewAttachments[] = { a_ImageViews[i] };

            VkFramebufferCreateInfo l_FramebufferCreateInfo{};
            l_FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            l_FramebufferCreateInfo.renderPass = m_RenderPass;
            l_FramebufferCreateInfo.attachmentCount = 1;
            l_FramebufferCreateInfo.pAttachments = l_ImageViewAttachments;
            l_FramebufferCreateInfo.width = swapchain.GetExtent().width;
            l_FramebufferCreateInfo.height = swapchain.GetExtent().height;
            l_FramebufferCreateInfo.layers = 1;

            Utilities::VulkanUtilities::VKCheckStrict(vkCreateFramebuffer(device.GetDevice(), &l_FramebufferCreateInfo, nullptr, &m_Framebuffers[i]), "vkCreateFramebuffer");
        }

        TR_CORE_INFO("MainPass create framebuffers");
    }

    void MainPass::DestroySwapchainResources(VulkanDevice& device)
    {
        TR_CORE_INFO("MainPass destroy swapchain resources");

        if (!device.GetDevice())
        {
            m_Framebuffers.clear();
            m_RenderPass = VK_NULL_HANDLE;

            return;
        }

        for (auto it_Framebuffer : m_Framebuffers)
        {
            if (it_Framebuffer)
            {
                vkDestroyFramebuffer(device.GetDevice(), it_Framebuffer, nullptr);
            }
        }

        m_Framebuffers.clear();

        if (m_RenderPass)
        {
            vkDestroyRenderPass(device.GetDevice(), m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }
    }

    void MainPass::CreatePipeline(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources)
    {
        VulkanPipeline::GraphicsPipelineDescription l_GraphicsPipelineDescription{};
        l_GraphicsPipelineDescription.VertexShaderPath = s_DefaultVertShader;
        l_GraphicsPipelineDescription.FragmentShaderPath = s_DefaultFragShader;
        l_GraphicsPipelineDescription.Extent = swapchain.GetExtent();
        l_GraphicsPipelineDescription.CullMode = VK_CULL_MODE_NONE;
        l_GraphicsPipelineDescription.FrontFace = VK_FRONT_FACE_CLOCKWISE;
        l_GraphicsPipelineDescription.PipelineCachePath = "pipeline_cache.bin";
        l_GraphicsPipelineDescription.PushConstantSize = sizeof(DrawPushConstant);
        l_GraphicsPipelineDescription.PushConstantStages = VK_SHADER_STAGE_VERTEX_BIT;

        std::array<VkDescriptorSetLayout, 1> l_DescriptorLayouts = { frameResources.GetGlobalSetLayout() };
        std::span<const VkDescriptorSetLayout> l_LayoutSpan = l_DescriptorLayouts[0] != VK_NULL_HANDLE
            ? std::span<const VkDescriptorSetLayout>(l_DescriptorLayouts) : std::span<const VkDescriptorSetLayout>();

        TR_CORE_INFO("MainPass create pipeline");
        m_Pipeline.Initialize(device, m_RenderPass, l_GraphicsPipelineDescription, l_LayoutSpan);

        m_Extent = l_GraphicsPipelineDescription.Extent;
    }

    void MainPass::CreateGlobalUniformBuffers(VulkanDevice& device, VulkanFrameResources& frameResources)
    {
        if (!m_GlobalUniformBuffers.empty())
        {
            return;
        }

        const uint32_t l_FramesInFlight = frameResources.GetFramesInFlight();
        m_GlobalUniformBuffers.resize(l_FramesInFlight);

        for (uint32_t it_Frame = 0; it_Frame < l_FramesInFlight; ++it_Frame)
        {
            m_GlobalUniformBuffers[it_Frame] = VulkanResources::CreateBuffer(device, sizeof(GlobalUniformData),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
    }

    void MainPass::DestroyGlobalUniformBuffers(VulkanDevice& device)
    {
        for (auto& it_Buffer : m_GlobalUniformBuffers)
        {
            VulkanResources::DestroyBuffer(device, it_Buffer);
        }

        m_GlobalUniformBuffers.clear();
    }

    void MainPass::DestroySwapchainResources(VulkanDevice& device, VulkanRenderer& renderer)
    {
        TR_CORE_INFO("MainPass destroy swapchain resources");

        std::vector<VkFramebuffer> l_Framebuffers = std::move(m_Framebuffers);
        VkRenderPass l_RenderPass = m_RenderPass;
        VulkanDevice* l_Device = &device;

        m_Framebuffers.clear();
        m_RenderPass = VK_NULL_HANDLE;

        renderer.SubmitResourceFree([l_Device, l_Framebuffers = std::move(l_Framebuffers), l_RenderPass]() mutable
        {
            if (!l_Device || !l_Device->GetDevice())
            {
                return;
            }

            for (auto it_Framebuffer : l_Framebuffers)
            {
                if (it_Framebuffer)
                {
                    vkDestroyFramebuffer(l_Device->GetDevice(), it_Framebuffer, nullptr);
                }
            }

            if (l_RenderPass)
            {
                vkDestroyRenderPass(l_Device->GetDevice(), l_RenderPass, nullptr);
            }
        });
    }
}