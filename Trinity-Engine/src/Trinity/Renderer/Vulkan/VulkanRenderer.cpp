#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"

#include "Trinity/Platform/Window.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>

namespace Trinity
{
	VulkanRenderer::VulkanRenderer() : Renderer(RendererAPI::VULKAN)
	{

	}

	VulkanRenderer::~VulkanRenderer()
	{

	}

	void VulkanRenderer::SetWindow(Window& window)
	{
		m_Window = &window;
	}

	void VulkanRenderer::Initialize()
	{
		TR_CORE_TRACE("Initializing Vulkan");

		if (!m_Window)
		{
			TR_CORE_CRITICAL("VulkanRenderer::Initialize called without a window");

			std::abort();
		}

		m_Context.Initialize(reinterpret_cast<HWND>(m_Window->GetNativeHandle().Window), reinterpret_cast<HINSTANCE>(m_Window->GetNativeHandle().Instance));
		m_Device.Initialize(m_Context);
		m_Swapchain.Initialize(m_Context, m_Device, m_Window->GetWidth(), m_Window->GetHeight());
		m_Sync.Initialize(m_Context, m_Device, m_FramesInFlight, m_Swapchain.GetImageCount());
		m_Command.Initialize(m_Context, m_Device, m_FramesInFlight);
		m_Pipeline.Initialize(m_Context, m_Device, m_Swapchain.GetImageFormat(), m_VertexShaderPath, m_FragmentShaderPath);
		m_SwapchainImageLayouts.assign(m_Swapchain.GetImageCount(), VK_IMAGE_LAYOUT_UNDEFINED);

		TR_CORE_TRACE("Vulkan Initialized");
	}

	void VulkanRenderer::Shutdown()
	{
		TR_CORE_TRACE("Shutting Down Vulkan");

		if (m_Device.GetDevice() != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_Device.GetDevice());
		}

		m_Pipeline.Shutdown();
		m_Command.Shutdown();
		m_Sync.Shutdown();
		m_Swapchain.Shutdown();
		m_Device.Shutdown();
		m_Context.Shutdown();

		TR_CORE_TRACE("Vulkan Shutdown Complete");
	}

	void VulkanRenderer::Resize(uint32_t width, uint32_t height)
	{
		RecreateSwapchain(width, height);
	}

	void VulkanRenderer::RecreateSwapchain(uint32_t width, uint32_t height)
	{
		if (m_Device.GetDevice() == VK_NULL_HANDLE)
		{
			return;
		}

		vkDeviceWaitIdle(m_Device.GetDevice());

		m_Swapchain.Recreate(width, height);
		m_Sync.OnSwapchainRecreated(m_Swapchain.GetImageCount());
		m_Pipeline.Recreate(m_Swapchain.GetImageFormat());
		m_SwapchainImageLayouts.assign(m_Swapchain.GetImageCount(), VK_IMAGE_LAYOUT_UNDEFINED);
	}

	void VulkanRenderer::BeginFrame()
	{
		if (!m_Window || m_Window->IsMinimized())
		{
			m_FrameBegun = false;

			return;
		}

		m_FrameBegun = false;

		m_Sync.WaitForFrameFence(m_CurrentFrameIndex);

		const VkSemaphore l_ImageAvailable = m_Sync.GetImageAvailableSemaphore(m_CurrentFrameIndex);

		VkResult l_AcquireResult = m_Swapchain.AcquireNextImageIndex(l_ImageAvailable, m_CurrentImageIndex);
		if (l_AcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain(m_Window->GetWidth(), m_Window->GetHeight());

			return;
		}

		if (l_AcquireResult != VK_SUCCESS && l_AcquireResult != VK_SUBOPTIMAL_KHR)
		{
			TR_CORE_CRITICAL("vkAcquireNextImageKHR failed (VkResult = {})", static_cast<int>(l_AcquireResult));

			std::abort();
		}

		const VkFence l_ImageFence = m_Sync.GetImageInFlightFence(m_CurrentImageIndex);
		if (l_ImageFence != VK_NULL_HANDLE)
		{
			Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device.GetDevice(), 1, &l_ImageFence, VK_TRUE, UINT64_MAX), "Failed vkWaitForFences");
		}

		const VkFence l_CurrentFrameFence = m_Sync.GetInFlightFence(m_CurrentFrameIndex);
		m_Sync.SetImageInFlightFence(m_CurrentImageIndex, l_CurrentFrameFence);

		m_Sync.ResetFrameFence(m_CurrentFrameIndex);
		m_Command.Reset(m_CurrentFrameIndex);

		m_Command.Begin(m_CurrentFrameIndex);

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

		TransitionSwapchainImage(l_CommandBuffer, m_CurrentImageIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkClearValue l_ClearValue{};
		l_ClearValue.color.float32[0] = 0.05f;
		l_ClearValue.color.float32[1] = 0.05f;
		l_ClearValue.color.float32[2] = 0.05f;
		l_ClearValue.color.float32[3] = 1.0f;

		VkRenderingAttachmentInfo l_ColorAttachment{};
		l_ColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		l_ColorAttachment.imageView = m_Swapchain.GetImageViews()[m_CurrentImageIndex];
		l_ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		l_ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_ColorAttachment.clearValue = l_ClearValue;

		VkRenderingInfo l_RenderingInfo{};
		l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		l_RenderingInfo.renderArea.offset = { 0, 0 };
		l_RenderingInfo.renderArea.extent = m_Swapchain.GetExtent();
		l_RenderingInfo.layerCount = 1;
		l_RenderingInfo.colorAttachmentCount = 1;
		l_RenderingInfo.pColorAttachments = &l_ColorAttachment;

		vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);

		const VkExtent2D l_Extent = m_Swapchain.GetExtent();

		VkViewport l_Viewport{};
		l_Viewport.x = 0.0f;
		l_Viewport.y = 0.0f;
		l_Viewport.width = static_cast<float>(l_Extent.width);
		l_Viewport.height = static_cast<float>(l_Extent.height);
		l_Viewport.minDepth = 0.0f;
		l_Viewport.maxDepth = 1.0f;

		VkRect2D l_Scissor{};
		l_Scissor.offset = { 0, 0 };
		l_Scissor.extent = l_Extent;

		vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);
		vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);

		// Bind + draw triangle
		m_Pipeline.Bind(l_CommandBuffer);
		vkCmdDraw(l_CommandBuffer, 3, 1, 0, 0);

		m_FrameBegun = true;
	}

	void VulkanRenderer::EndFrame()
	{
		if (!m_FrameBegun)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

		vkCmdEndRendering(l_CommandBuffer);

		TransitionSwapchainImage(l_CommandBuffer, m_CurrentImageIndex, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		m_Command.End(m_CurrentFrameIndex);

		const VkSemaphore l_ImageAvailable = m_Sync.GetImageAvailableSemaphore(m_CurrentFrameIndex);
		const VkSemaphore l_RenderFinished = m_Sync.GetRenderFinishedSemaphore(m_CurrentImageIndex);
		const VkFence l_InFlightFence = m_Sync.GetInFlightFence(m_CurrentFrameIndex);

		VkPipelineStageFlags l_WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo l_SubmitInfo{};
		l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		l_SubmitInfo.waitSemaphoreCount = 1;
		l_SubmitInfo.pWaitSemaphores = &l_ImageAvailable;
		l_SubmitInfo.pWaitDstStageMask = &l_WaitStage;
		l_SubmitInfo.commandBufferCount = 1;
		l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;
		l_SubmitInfo.signalSemaphoreCount = 1;
		l_SubmitInfo.pSignalSemaphores = &l_RenderFinished;

		Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &l_SubmitInfo, l_InFlightFence), "Failed vkQueueSubmit");

		VkResult l_PresentResult = m_Swapchain.Present(m_Device.GetPresentQueue(), l_RenderFinished, m_CurrentImageIndex);
		if (l_PresentResult == VK_ERROR_OUT_OF_DATE_KHR || l_PresentResult == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapchain(m_Window->GetWidth(), m_Window->GetHeight());
		}
		else if (l_PresentResult != VK_SUCCESS)
		{
			TR_CORE_CRITICAL("vkQueuePresentKHR failed (VkResult = {})", static_cast<int>(l_PresentResult));
			std::abort();
		}

		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_FramesInFlight;
		m_FrameBegun = false;
	}

	void VulkanRenderer::TransitionSwapchainImage(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkImageLayout newLayout)
	{
		if (imageIndex >= m_SwapchainImageLayouts.size())
		{
			TR_CORE_CRITICAL("Swapchain image index out of range in TransitionSwapchainImage");

			std::abort();
		}

		const VkImageLayout l_OldLayout = m_SwapchainImageLayouts[imageIndex];
		if (l_OldLayout == newLayout)
		{
			return;
		}

		VkImageMemoryBarrier2 l_Barrier{};
		l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		l_Barrier.oldLayout = l_OldLayout;
		l_Barrier.newLayout = newLayout;
		l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		l_Barrier.image = m_Swapchain.GetImages()[imageIndex];
		l_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_Barrier.subresourceRange.baseMipLevel = 0;
		l_Barrier.subresourceRange.levelCount = 1;
		l_Barrier.subresourceRange.baseArrayLayer = 0;
		l_Barrier.subresourceRange.layerCount = 1;

		if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
			l_Barrier.srcAccessMask = VK_ACCESS_2_NONE;
			l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			l_Barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			l_Barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
			l_Barrier.dstAccessMask = VK_ACCESS_2_NONE;
		}
		else
		{
			l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			l_Barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
			l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			l_Barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
		}

		VkDependencyInfo l_Dependency{};
		l_Dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		l_Dependency.imageMemoryBarrierCount = 1;
		l_Dependency.pImageMemoryBarriers = &l_Barrier;

		vkCmdPipelineBarrier2(commandBuffer, &l_Dependency);

		m_SwapchainImageLayouts[imageIndex] = newLayout;
	}
}