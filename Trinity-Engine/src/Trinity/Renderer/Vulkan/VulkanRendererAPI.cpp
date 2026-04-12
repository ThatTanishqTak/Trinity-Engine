#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanFramebuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanShader.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanSampler.h"
#include "Trinity/Platform/Window/Window.h"

#include <limits>

namespace Trinity
{
    VulkanRendererAPI::VulkanRendererAPI()
    {
        m_Backend = RendererBackend::Vulkan;
    }

    VulkanRendererAPI::~VulkanRendererAPI()
    {

    }

    void VulkanRendererAPI::Initialize(Window& window, const RendererAPISpecification& specification)
    {
        m_MaxFramesInFlight = specification.MaxFramesInFlight;

        m_Instance.Initialize(window, specification.EnableValidation);
        m_Device.Initialize(m_Instance, specification.EnableValidation);

        if (m_Instance.IsValidationEnabled())
        {
            m_Debug.Initialize(m_Instance.GetInstance());
        }

        m_Allocator.Initialize(m_Device);
        m_Swapchain.Initialize(m_Device, window.GetWidth(), window.GetHeight(), true);
        m_CommandPool.Initialize(m_Device, m_MaxFramesInFlight);
        m_SyncObjects.Initialize(m_Device.GetDevice(), m_MaxFramesInFlight);

        TR_CORE_INFO("Vulkan renderer initialized.");
    }

    void VulkanRendererAPI::Shutdown()
    {
        vkDeviceWaitIdle(m_Device.GetDevice());

        m_SyncObjects.Shutdown();
        m_CommandPool.Shutdown();
        m_Swapchain.Shutdown();
        m_Allocator.Shutdown();
        m_Debug.Shutdown();
        m_Device.Shutdown();
        m_Instance.Shutdown();

        TR_CORE_INFO("Vulkan renderer shut down.");
    }

    bool VulkanRendererAPI::BeginFrame()
    {
        m_SyncObjects.WaitForFence(m_CurrentFrameIndex);

        VkResult l_Result = vkAcquireNextImageKHR(m_Device.GetDevice(), m_Swapchain.GetSwapchain(), std::numeric_limits<uint64_t>::max(), m_SyncObjects.GetImageAvailableSemaphore(m_CurrentFrameIndex), VK_NULL_HANDLE, &m_CurrentImageIndex);

        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_FramebufferResized = false;
            return false;
        }

        if (l_Result != VK_SUCCESS && l_Result != VK_SUBOPTIMAL_KHR)
        {
            TR_CORE_ERROR("Failed to acquire swapchain image.");
            return false;
        }

        m_SyncObjects.ResetFence(m_CurrentFrameIndex);
        m_CommandPool.ResetCommandBuffer(m_CurrentFrameIndex);
        m_CommandPool.BeginCommandBuffer(m_CurrentFrameIndex);

        VkCommandBuffer l_Cmd = GetCurrentCommandBuffer();

        TransitionImageLayout(l_Cmd, m_Swapchain.GetImages()[m_CurrentImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        m_FrameStarted = true;

        return true;
    }

    void VulkanRendererAPI::EndFrame()
    {
        if (!m_FrameStarted)
        {
            return;
        }

        VkCommandBuffer l_Cmd = GetCurrentCommandBuffer();

        TransitionImageLayout(l_Cmd, m_Swapchain.GetImages()[m_CurrentImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        m_CommandPool.EndCommandBuffer(m_CurrentFrameIndex);

        m_FrameStarted = false;
    }

    void VulkanRendererAPI::Present()
    {
        VkSemaphore l_WaitSemaphores[] = { m_SyncObjects.GetImageAvailableSemaphore(m_CurrentFrameIndex) };
        VkSemaphore l_SignalSemaphores[] = { m_SyncObjects.GetRenderFinishedSemaphore(m_CurrentFrameIndex) };
        VkPipelineStageFlags l_WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkCommandBuffer l_Cmd = m_CommandPool.GetCommandBuffer(m_CurrentFrameIndex);

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.waitSemaphoreCount = 1;
        l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores;
        l_SubmitInfo.pWaitDstStageMask = l_WaitStages;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &l_Cmd;
        l_SubmitInfo.signalSemaphoreCount = 1;
        l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores;

        VulkanUtilities::VKCheck(vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &l_SubmitInfo, m_SyncObjects.GetInFlightFence(m_CurrentFrameIndex)), "Failed vkQueueSubmit");

        VkSwapchainKHR l_Swapchains[] = { m_Swapchain.GetSwapchain() };

        VkPresentInfoKHR l_PresentInfo{};
        l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        l_PresentInfo.waitSemaphoreCount = 1;
        l_PresentInfo.pWaitSemaphores = l_SignalSemaphores;
        l_PresentInfo.swapchainCount = 1;
        l_PresentInfo.pSwapchains = l_Swapchains;
        l_PresentInfo.pImageIndices = &m_CurrentImageIndex;

        VkResult l_Result = vkQueuePresentKHR(m_Device.GetPresentQueue(), &l_PresentInfo);

        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR || l_Result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
        {
            m_FramebufferResized = false;
            m_Swapchain.Recreate(m_Swapchain.GetExtent().width, m_Swapchain.GetExtent().height);
        }
        else if (l_Result != VK_SUCCESS)
        {
            TR_CORE_ERROR("Failed to present swapchain image.");
        }

        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFramesInFlight;
    }

    void VulkanRendererAPI::WaitIdle()
    {
        vkDeviceWaitIdle(m_Device.GetDevice());
    }

    void VulkanRendererAPI::OnWindowResize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        m_FramebufferResized = true;
        vkDeviceWaitIdle(m_Device.GetDevice());
        m_Swapchain.Recreate(width, height);
    }

    uint32_t VulkanRendererAPI::GetSwapchainWidth() const
    {
        return m_Swapchain.GetExtent().width;
    }

    uint32_t VulkanRendererAPI::GetSwapchainHeight() const
    {
        return m_Swapchain.GetExtent().height;
    }

    VkCommandBuffer VulkanRendererAPI::GetCurrentCommandBuffer() const
    {
        return m_CommandPool.GetCommandBuffer(m_CurrentFrameIndex);
    }

    std::shared_ptr<Buffer> VulkanRendererAPI::CreateBuffer(const BufferSpecification& specification)
    {
        return std::make_shared<VulkanBuffer>(m_Device.GetDevice(), m_Allocator.GetAllocator(), specification);
    }

    std::shared_ptr<Texture> VulkanRendererAPI::CreateTexture(const TextureSpecification& specification)
    {
        return std::make_shared<VulkanTexture>(m_Device.GetDevice(), m_Allocator.GetAllocator(), specification);
    }

    std::shared_ptr<Framebuffer> VulkanRendererAPI::CreateFramebuffer(const FramebufferSpecification& specification)
    {
        return std::make_shared<VulkanFramebuffer>(m_Device.GetDevice(), m_Allocator.GetAllocator(), specification);
    }

    std::shared_ptr<Shader> VulkanRendererAPI::CreateShader(const ShaderSpecification& specification)
    {
        return std::make_shared<VulkanShader>(m_Device.GetDevice(), specification);
    }

    std::shared_ptr<Pipeline> VulkanRendererAPI::CreatePipeline(const PipelineSpecification& specification)
    {
        return std::make_shared<VulkanPipeline>(m_Device.GetDevice(), specification);
    }

    std::shared_ptr<Sampler> VulkanRendererAPI::CreateSampler(const SamplerSpecification& specification)
    {
        return std::make_shared<VulkanSampler>(m_Device.GetDevice(), specification);
    }

    void VulkanRendererAPI::TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) const
    {
        VkImageMemoryBarrier2 l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        l_Barrier.oldLayout = oldLayout;
        l_Barrier.newLayout = newLayout;
        l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.image = image;
        l_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        l_Barrier.subresourceRange.baseMipLevel = 0;
        l_Barrier.subresourceRange.levelCount = 1;
        l_Barrier.subresourceRange.baseArrayLayer = 0;
        l_Barrier.subresourceRange.layerCount = 1;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            l_Barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            l_Barrier.dstAccessMask = 0;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            l_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            l_Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        }
        else
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            l_Barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        }

        VkDependencyInfo l_DependencyInfo{};
        l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_DependencyInfo.imageMemoryBarrierCount = 1;
        l_DependencyInfo.pImageMemoryBarriers = &l_Barrier;

        vkCmdPipelineBarrier2(cmd, &l_DependencyInfo);
    }
}