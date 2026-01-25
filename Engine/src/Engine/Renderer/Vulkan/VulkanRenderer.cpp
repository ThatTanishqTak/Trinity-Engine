#include "Engine/Renderer/Vulkan/VulkanRenderer.h"

#include "Engine/Platform/Window.h"
#include "Engine/Utilities/Utilities.h"

#include <GLFW/glfw3.h>

namespace Engine
{
    VulkanRenderer::~VulkanRenderer()
    {
        Shutdown();
    }

    void VulkanRenderer::SetClearColor(const glm::vec4& a_Color)
    {
        // Stored and applied during command buffer recording.
        m_ClearColor = a_Color;
    }

    void VulkanRenderer::Clear()
    {
        // Vulkan clear happens via render pass loadOp (we use CLEAR) and the clear value.
        // This function exists for API symmetry; RecordCommandBuffer uses m_ClearColor.
        m_ClearRequested = true;
    }

    void VulkanRenderer::DrawCube(const glm::vec3& a_Size, const glm::vec3& a_Position, const glm::vec4& a_Tint)
    {
        // For now, queue requests and let RecordCommandBuffer emit the draw calls.
        // Real cube rendering will be wired once the pipeline/shaders support it.
        m_PendingCubes.push_back({ a_Size, a_Position, a_Tint });
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
        m_MainPass.Initialize(m_Device, m_Swapchain, m_FrameResources);

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

            m_MainPass.OnDestroy(m_Device);
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
        m_MainPass.RecordCommandBuffer(l_CommandBuffer, m_ImageIndex, (uint32_t)m_CurrentFrame, m_ClearColor, m_PendingCubes);
        m_PendingCubes.clear();
        m_ClearRequested = false;

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

    void VulkanRenderer::RecreateSwapchain()
    {
        if (!m_Device.GetDevice())
        {
            return;
        }

        vkDeviceWaitIdle(m_Device.GetDevice());

        m_MainPass.OnResize(m_Device, m_Swapchain, m_FrameResources);

        m_FrameResources.OnSwapchainRecreated(m_Swapchain.GetImages().size());
    }
}