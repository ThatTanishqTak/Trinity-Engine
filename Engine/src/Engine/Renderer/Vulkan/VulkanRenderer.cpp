#include "Engine/Renderer/Vulkan/VulkanRenderer.h"

#include "Engine/Platform/Window.h"
#include "Engine/Utilities/Utilities.h"

#include <GLFW/glfw3.h>

#include <array>
#include <span>
#include <stdexcept>
#include <string>

namespace Engine
{
    // Change these to wherever your compiled SPIR-V actually lives.
    static const char* s_DefaultVertShader = "Assets/Shaders/Simple.vert.spv";
    static const char* s_DefaultFragShader = "Assets/Shaders/Simple.frag.spv";

    VulkanRenderer::~VulkanRenderer()
    {
        Shutdown();
    }

    void VulkanRenderer::Initialize(Window& window)
    {
        TR_CORE_INFO("------ INITIALIZING RENDERER -------");

        Shutdown();

        m_Window = &window;
        m_GLFWWindow = (GLFWwindow*)window.GetNativeWindow();
        m_LastVSync = window.IsVSync();

        m_Context.Initialize(window);
        m_Device.Initialize(m_Context);
        m_FrameResources.Initialize(m_Device, (uint32_t)s_MaxFramesInFlight);
        m_Swapchain.Initialize(m_Context, m_Device, window);
        m_FrameResources.OnSwapchainRecreated(m_Swapchain.GetImages().size());

        CreateRenderPass();
        CreateFramebuffers();
        CreatePipeline();

        m_CurrentFrame = 0;
        m_ImageIndex = 0;

        m_Initialized = true;

        TR_CORE_INFO("------ RENDERER INITIALIZED -------");
    }

    void VulkanRenderer::Shutdown()
    {
        if (!m_Initialized)
        {
            m_Context.Shutdown();
            return;
        }

        if (m_Device.GetDevice())
        {
            vkDeviceWaitIdle(m_Device.GetDevice());

            // Pipeline depends on render pass and swapchain extent, kill it before swapchain-related cleanup.
            m_Pipeline.Shutdown(m_Device);
            CleanupSwapchainDependents();
            m_Swapchain.Shutdown();
            m_FrameResources.Shutdown(m_Device);
            m_Device.Shutdown();
        }

        m_Context.Shutdown();

        m_Window = nullptr;
        m_GLFWWindow = nullptr;

        m_FramebufferResized = false;
        m_FrameInProgress = false;
        m_Initialized = false;

        m_CurrentFrame = 0;
        m_ImageIndex = 0;
    }

    void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
    {
        m_FramebufferResized = true;

        if (m_Window)
        {
            m_Window->SetWidth(width);
            m_Window->SetHeight(height);
        }
    }

    void VulkanRenderer::BeginFrame()
    {
        if (!m_Initialized || !m_Device.GetDevice() || !m_Swapchain.IsValid() || !m_FrameResources.IsValid())
        {
            return;
        }

        m_FrameInProgress = false;

        const bool l_CurrentVSync = m_Window ? m_Window->IsVSync() : true;
        if (l_CurrentVSync != m_LastVSync)
        {
            m_LastVSync = l_CurrentVSync;
            RecreateSwapchain();

            return;
        }

        m_FrameResources.WaitForFrameFence(m_Device, (uint32_t)m_CurrentFrame, UINT64_MAX);

        VkSemaphore l_ImageAvailable = m_FrameResources.GetImageAvailableSemaphore((uint32_t)m_CurrentFrame);

        VkResult l_AcquireResult = m_Swapchain.AcquireNextImage(UINT64_MAX, l_ImageAvailable, VK_NULL_HANDLE, m_ImageIndex);

        if (l_AcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain();

            return;
        }

        if (l_AcquireResult != VK_SUCCESS && l_AcquireResult != VK_SUBOPTIMAL_KHR)
        {
            Utilities::VulkanUtilities::VKCheckStrict(l_AcquireResult, "vkAcquireNextImageKHR");
        }

        m_FrameResources.WaitForImageFenceIfNeeded(m_Device, m_ImageIndex, UINT64_MAX);

        VkFence l_FrameFence = m_FrameResources.GetInFlightFence((uint32_t)m_CurrentFrame);
        m_FrameResources.MarkImageInFlight(m_ImageIndex, l_FrameFence);
        m_FrameResources.ResetForFrame(m_Device, (uint32_t)m_CurrentFrame);

        VkCommandBuffer l_CommandBuffer = m_FrameResources.GetCommandBuffer((uint32_t)m_CurrentFrame);
        RecordCommandBuffer(l_CommandBuffer, m_ImageIndex);

        VkSemaphore l_WaitSemaphores[] = { l_ImageAvailable };
        VkPipelineStageFlags l_WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSemaphore l_RenderFinished = m_FrameResources.GetRenderFinishedSemaphore((uint32_t)m_CurrentFrame);
        VkSemaphore l_SignalSemaphores[] = { l_RenderFinished };

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.waitSemaphoreCount = 1;
        l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores;
        l_SubmitInfo.pWaitDstStageMask = l_WaitStages;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;
        l_SubmitInfo.signalSemaphoreCount = 1;
        l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores;

        Utilities::VulkanUtilities::VKCheckStrict(vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &l_SubmitInfo, l_FrameFence), "vkQueueSubmit");

        m_FrameInProgress = true;
    }

    void VulkanRenderer::EndFrame()
    {
        if (!m_FrameInProgress || !m_Swapchain.IsValid() || !m_FrameResources.IsValid())
        {
            return;
        }

        VkSemaphore l_WaitSemaphores[] = { m_FrameResources.GetRenderFinishedSemaphore((uint32_t)m_CurrentFrame) };
        VkSwapchainKHR l_SwapchainHandle = m_Swapchain.GetSwapchain();

        VkPresentInfoKHR l_PresentInfo{};
        l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        l_PresentInfo.waitSemaphoreCount = 1;
        l_PresentInfo.pWaitSemaphores = l_WaitSemaphores;
        l_PresentInfo.swapchainCount = 1;
        l_PresentInfo.pSwapchains = &l_SwapchainHandle;
        l_PresentInfo.pImageIndices = &m_ImageIndex;

        VkResult l_PresentResult = vkQueuePresentKHR(m_Device.GetPresentQueue(), &l_PresentInfo);

        if (l_PresentResult == VK_ERROR_OUT_OF_DATE_KHR || l_PresentResult == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
        {
            m_FramebufferResized = false;
            RecreateSwapchain();
        }
        else if (l_PresentResult != VK_SUCCESS)
        {
            Utilities::VulkanUtilities::VKCheckStrict(l_PresentResult, "vkQueuePresentKHR");
        }

        m_FrameInProgress = false;
        m_CurrentFrame = (m_CurrentFrame + 1) % s_MaxFramesInFlight;
    }

    void VulkanRenderer::CreateRenderPass()
    {
        if (m_RenderPass)
        {
            vkDestroyRenderPass(m_Device.GetDevice(), m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }

        VkAttachmentDescription l_ColorAttachmentDescription{};
        l_ColorAttachmentDescription.format = m_Swapchain.GetImageFormat();
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

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateRenderPass(m_Device.GetDevice(), &l_RenderPassCreateInfo, nullptr, &m_RenderPass), "vkCreateRenderPass");
    }

    void VulkanRenderer::CreateFramebuffers()
    {
        for (auto it_Framebuffer : m_Framebuffers)
        {
            if (it_Framebuffer)
            {
                vkDestroyFramebuffer(m_Device.GetDevice(), it_Framebuffer, nullptr);
            }
        }
        m_Framebuffers.clear();

        const auto& a_ImageViews = m_Swapchain.GetImageViews();
        m_Framebuffers.resize(a_ImageViews.size());

        for (size_t i = 0; i < a_ImageViews.size(); ++i)
        {
            VkImageView l_ImageViewAttachments[] = { a_ImageViews[i] };

            VkFramebufferCreateInfo l_FramebufferCreateInfo{};
            l_FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            l_FramebufferCreateInfo.renderPass = m_RenderPass;
            l_FramebufferCreateInfo.attachmentCount = 1;
            l_FramebufferCreateInfo.pAttachments = l_ImageViewAttachments;
            l_FramebufferCreateInfo.width = m_Swapchain.GetExtent().width;
            l_FramebufferCreateInfo.height = m_Swapchain.GetExtent().height;
            l_FramebufferCreateInfo.layers = 1;

            Utilities::VulkanUtilities::VKCheckStrict(vkCreateFramebuffer(m_Device.GetDevice(), &l_FramebufferCreateInfo, nullptr, &m_Framebuffers[i]), "vkCreateFramebuffer");
        }
    }

    void VulkanRenderer::CreatePipeline()
    {
        VulkanPipeline::GraphicsPipelineDescription l_GraphicsPipelineDescription{};
        l_GraphicsPipelineDescription.VertexShaderPath = s_DefaultVertShader;
        l_GraphicsPipelineDescription.FragmentShaderPath = s_DefaultFragShader;
        l_GraphicsPipelineDescription.Extent = m_Swapchain.GetExtent();
        l_GraphicsPipelineDescription.CullMode = VK_CULL_MODE_NONE;
        l_GraphicsPipelineDescription.FrontFace = VK_FRONT_FACE_CLOCKWISE;
        l_GraphicsPipelineDescription.PipelineCachePath = "pipeline_cache.bin";

        // If you want to make this configurable later, go wild.
        std::array<VkDescriptorSetLayout, 1> l_DescriptorLayouts = { m_FrameResources.GetDescriptorSetLayout() };
        std::span<const VkDescriptorSetLayout> l_LayoutSpan = m_FrameResources.HasDescriptors() ? std::span<const VkDescriptorSetLayout>(l_DescriptorLayouts) : std::span<const VkDescriptorSetLayout>();

        m_Pipeline.Initialize(m_Device, m_RenderPass, l_GraphicsPipelineDescription, l_LayoutSpan);
    }

    void VulkanRenderer::RecordCommandBuffer(VkCommandBuffer command, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo l_CommandBufferBeginInfo{};
        l_CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        Utilities::VulkanUtilities::VKCheckStrict(vkBeginCommandBuffer(command, &l_CommandBufferBeginInfo), "vkBeginCommandBuffer");

        VkClearValue l_ClearColor{};
        l_ClearColor.color.float32[0] = 0.05f;
        l_ClearColor.color.float32[1] = 0.05f;
        l_ClearColor.color.float32[2] = 0.05f;
        l_ClearColor.color.float32[3] = 1.0f;

        VkRenderPassBeginInfo l_RenderPassBeginInfo{};
        l_RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        l_RenderPassBeginInfo.renderPass = m_RenderPass;
        l_RenderPassBeginInfo.framebuffer = m_Framebuffers[imageIndex];
        l_RenderPassBeginInfo.renderArea.offset = { 0, 0 };
        l_RenderPassBeginInfo.renderArea.extent = m_Swapchain.GetExtent();
        l_RenderPassBeginInfo.clearValueCount = 1;
        l_RenderPassBeginInfo.pClearValues = &l_ClearColor;

        vkCmdBeginRenderPass(command, &l_RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (m_Pipeline.IsValid())
        {
            VkViewport l_Viewport{};
            l_Viewport.x = 0.0f;
            l_Viewport.y = 0.0f;
            l_Viewport.width = (float)m_Swapchain.GetExtent().width;
            l_Viewport.height = (float)m_Swapchain.GetExtent().height;
            l_Viewport.minDepth = 0.0f;
            l_Viewport.maxDepth = 1.0f;

            VkRect2D l_Scissor{};
            l_Scissor.offset = { 0, 0 };
            l_Scissor.extent = m_Swapchain.GetExtent();

            vkCmdSetViewport(command, 0, 1, &l_Viewport);
            vkCmdSetScissor(command, 0, 1, &l_Scissor);

            vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.GetPipeline());

            if (m_FrameResources.HasDescriptors())
            {
                VkDescriptorSet l_DescriptorSet = m_FrameResources.GetDescriptorSet((uint32_t)m_CurrentFrame);
                if (l_DescriptorSet != VK_NULL_HANDLE)
                {
                    // Bind per-frame descriptor set at set 0 to match the pipeline layout.
                    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.GetPipelineLayout(), 0, 1, &l_DescriptorSet, 0, nullptr);
                }
            }

            // The shaders must be written to use gl_VertexIndex (no vertex buffers).
            vkCmdDraw(command, 3, 1, 0, 0);
        }

        vkCmdEndRenderPass(command);

        Utilities::VulkanUtilities::VKCheckStrict(vkEndCommandBuffer(command), "vkEndCommandBuffer");
    }

    void VulkanRenderer::CleanupSwapchainDependents()
    {
        for (auto it_Framebuffer : m_Framebuffers)
        {
            if (it_Framebuffer)
            {
                vkDestroyFramebuffer(m_Device.GetDevice(), it_Framebuffer, nullptr);
            }
        }
        m_Framebuffers.clear();

        if (m_RenderPass)
        {
            vkDestroyRenderPass(m_Device.GetDevice(), m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }
    }

    void VulkanRenderer::RecreateSwapchain()
    {
        if (!m_Device.GetDevice())
        {
            return;
        }

        vkDeviceWaitIdle(m_Device.GetDevice());

        // Kill pipeline first (depends on render pass + extent).
        m_Pipeline.Shutdown(m_Device);

        CleanupSwapchainDependents();
        m_Swapchain.Recreate();

        CreateRenderPass();
        CreateFramebuffers();
        CreatePipeline();

        m_FrameResources.OnSwapchainRecreated(m_Swapchain.GetImages().size());
    }
}