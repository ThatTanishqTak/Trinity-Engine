#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"

#include "Trinity/Platform/Window.h"
#include "Trinity/Utilities/Utilities.h"

#include <cstdlib>

namespace Trinity
{
	VulkanRenderer::VulkanRenderer()
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
		if (m_Window == nullptr)
		{
			TR_CORE_CRITICAL("VulkanRenderer::Initialize called without a Window");

			std::abort();
		}

		TR_CORE_TRACE("Initializing vulkan renderer");

		m_Instance.Initialize();
		m_Surface.Initialize(m_Instance, *m_Window);
		m_Device.Initialize(m_Instance.GetInstance(), m_Surface.GetSurface(), m_Instance.GetAllocator());
		m_Swapchain.Initialize(m_Device, m_Surface.GetSurface(), m_Instance.GetAllocator(), m_Window->IsVSync());
		m_Sync.Initialize(m_Device.GetDevice());
		m_Sync.Create(m_FramesInFlight, m_Swapchain.GetImageCount());
		m_RenderPass.Initialize(m_Device, m_Swapchain, m_Instance.GetAllocator());
		m_Framebuffers.Initialize(m_Device, m_Swapchain, m_RenderPass, m_Instance.GetAllocator());
		m_FrameResources.Initialize(m_Device, m_FramesInFlight, m_Instance.GetAllocator());
		m_Descriptors.Initialize(m_Device, m_FramesInFlight, m_Instance.GetAllocator());

		VulkanPipeline::GraphicsDescription l_PipelineDesc{};
		l_PipelineDesc.EnableDepthTest = true;
		l_PipelineDesc.EnableDepthWrite = true;
		l_PipelineDesc.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		m_Pipeline.SetGraphicsDescription(l_PipelineDesc);
		m_Pipeline.Initialize(m_Device, m_RenderPass, m_Descriptors, "Assets/Shaders/Simple.vert.spv", "Assets/Shaders/Simple.frag.spv", m_Instance.GetAllocator(), true);
		m_Command.Initialize(m_Device, m_FramesInFlight, m_Instance.GetAllocator());

		TR_CORE_TRACE("Vulkan renderer initialized");
	}

	void VulkanRenderer::Shutdown()
	{
		m_Command.Shutdown();
		m_Pipeline.Shutdown();
		m_Descriptors.Shutdown();
		m_FrameResources.Shutdown();
		m_Framebuffers.Shutdown();
		m_RenderPass.Shutdown();
		m_Sync.Cleanup();
		m_Swapchain.Shutdown();
		m_Device.Shutdown();
		m_Surface.Shutdown(m_Instance);
		m_Instance.Shutdown();
	}

	void VulkanRenderer::Resize(uint32_t width, uint32_t height)
	{
		m_Swapchain.Recreate(width, height);

		m_Sync.Create(m_FramesInFlight, m_Swapchain.GetImageCount());

		m_RenderPass.Recreate(m_Swapchain);
		m_Framebuffers.Recreate(m_Swapchain, m_RenderPass);
		m_Pipeline.Recreate(m_RenderPass);
	}

	void VulkanRenderer::BeginFrame()
	{
		m_FrameBegun = false;

		const uint32_t l_FrameIndex = m_FrameResources.GetCurrentFrameIndex();

		m_Sync.WaitForFrameFence(l_FrameIndex);

		const VkSemaphore l_ImageAvailableSemaphore = m_Sync.GetImageAvailableSemaphore(l_FrameIndex);

		VkResult l_AcquireResult = m_Swapchain.AcquireNextImage(l_ImageAvailableSemaphore, m_CurrentImageIndex);
		if (l_AcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_Swapchain.Recreate();
			m_Sync.Create(m_FramesInFlight, m_Swapchain.GetImageCount());
			m_RenderPass.Recreate(m_Swapchain);
			m_Framebuffers.Recreate(m_Swapchain, m_RenderPass);
			m_Pipeline.Recreate(m_RenderPass);

			return;
		}

		if (l_AcquireResult != VK_SUCCESS && l_AcquireResult != VK_SUBOPTIMAL_KHR)
		{
			TR_CORE_CRITICAL("vkAcquireNextImageKHR failed with {}", (int)l_AcquireResult);

			std::abort();
		}

		const VkFence l_FrameFence = m_Sync.GetInFlightFence(l_FrameIndex);
		const VkFence l_ImageFence = m_Sync.GetImageInFlightFence(m_CurrentImageIndex);
		if (l_ImageFence != VK_NULL_HANDLE && l_ImageFence != l_FrameFence)
		{
			Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device.GetDevice(), 1, &l_ImageFence, VK_TRUE, UINT64_MAX), "Failed vkWaitForFences (image in flight)");
		}

		m_Sync.ResetFrameFence(l_FrameIndex);
		m_Sync.SetImageInFlightFence(m_CurrentImageIndex, l_FrameFence);

		VkCommandBuffer l_CommandBuffer = m_Command.BeginFrame(l_FrameIndex);

		VkImage l_Image = m_Swapchain.GetImage(m_CurrentImageIndex);

		VkImageSubresourceRange l_Range{};
		l_Range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_Range.baseMipLevel = 0;
		l_Range.levelCount = 1;
		l_Range.baseArrayLayer = 0;
		l_Range.layerCount = 1;

		VkImageLayout l_OldLayout = m_Swapchain.GetTrackedLayout(m_CurrentImageIndex);

		if (l_OldLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			VkImageMemoryBarrier l_ToColor{};
			l_ToColor.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			l_ToColor.oldLayout = l_OldLayout;
			l_ToColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			l_ToColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			l_ToColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			l_ToColor.image = l_Image;
			l_ToColor.subresourceRange = l_Range;
			l_ToColor.srcAccessMask = 0;
			l_ToColor.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			VkPipelineStageFlags l_SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			if (l_OldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
			{
				l_SrcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			}

			vkCmdPipelineBarrier(l_CommandBuffer, l_SrcStage, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &l_ToColor);

			m_Swapchain.SetTrackedLayout(m_CurrentImageIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}

		VkClearValue l_ClearValues[2]{};

		// Color clear
		l_ClearValues[0].color.float32[0] = 0.01f;
		l_ClearValues[0].color.float32[1] = 0.01f;
		l_ClearValues[0].color.float32[2] = 0.01f;
		l_ClearValues[0].color.float32[3] = 1.0f;

		l_ClearValues[1].depthStencil.depth = 1.0f;
		l_ClearValues[1].depthStencil.stencil = 0;

		VkRenderPassBeginInfo l_RPBegin{};
		l_RPBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		l_RPBegin.renderPass = m_RenderPass.GetRenderPass();
		l_RPBegin.framebuffer = m_Framebuffers.GetFramebuffer(m_CurrentImageIndex);
		l_RPBegin.renderArea.offset = { 0, 0 };
		l_RPBegin.renderArea.extent = m_Swapchain.GetExtent();
		l_RPBegin.clearValueCount = 2;
		l_RPBegin.pClearValues = l_ClearValues;

		vkCmdBeginRenderPass(l_CommandBuffer, &l_RPBegin, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport l_Viewport{};
		l_Viewport.x = 0.0f;
		l_Viewport.y = 0.0f;
		l_Viewport.width = static_cast<float>(m_Swapchain.GetExtent().width);
		l_Viewport.height = static_cast<float>(m_Swapchain.GetExtent().height);
		l_Viewport.minDepth = 0.0f;
		l_Viewport.maxDepth = 1.0f;

		VkRect2D l_Scissor{};
		l_Scissor.offset = { 0, 0 };
		l_Scissor.extent = m_Swapchain.GetExtent();

		vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);
		vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);

		vkCmdBindPipeline(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.GetPipeline());

		VkDescriptorSet l_GlobalSet = m_Descriptors.GetGlobalSet(l_FrameIndex);
		if (l_GlobalSet != VK_NULL_HANDLE)
		{
			vkCmdBindDescriptorSets(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.GetPipelineLayout(), 0, 1, &l_GlobalSet, 0, nullptr);
		}

		vkCmdDraw(l_CommandBuffer, 3, 1, 0, 0);

		m_FrameBegun = true;
	}

	void VulkanRenderer::EndFrame()
	{
		if (!m_FrameBegun)
		{
			return;
		}

		const uint32_t l_FrameIndex = m_FrameResources.GetCurrentFrameIndex();

		VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(l_FrameIndex);

		vkCmdEndRenderPass(l_CommandBuffer);
		m_Swapchain.SetTrackedLayout(m_CurrentImageIndex, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		m_Command.EndFrame(l_FrameIndex);

		const VkSemaphore l_ImageAvailableSemaphore = m_Sync.GetImageAvailableSemaphore(l_FrameIndex);
		const VkSemaphore l_RenderFinishedSemaphore = m_Sync.GetRenderFinishedSemaphore(m_CurrentImageIndex);
		const VkFence l_InFlightFence = m_Sync.GetInFlightFence(l_FrameIndex);

		VkSemaphore l_WaitSemaphores[] = { l_ImageAvailableSemaphore };

		VkPipelineStageFlags l_WaitStages[] =
		{
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
		};

		VkSemaphore l_SignalSemaphores[] = { l_RenderFinishedSemaphore };

		VkSubmitInfo l_SubmitInfo{};
		l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		l_SubmitInfo.waitSemaphoreCount = 1;
		l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores;
		l_SubmitInfo.pWaitDstStageMask = l_WaitStages;
		l_SubmitInfo.commandBufferCount = 1;
		l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;
		l_SubmitInfo.signalSemaphoreCount = 1;
		l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores;

		Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &l_SubmitInfo, l_InFlightFence), "Failed vkQueueSubmit");

		VkResult l_PresentResult = m_Swapchain.Present(m_Device.GetPresentQueue(), m_CurrentImageIndex);

		if (l_PresentResult == VK_ERROR_OUT_OF_DATE_KHR || l_PresentResult == VK_SUBOPTIMAL_KHR)
		{
			m_Swapchain.Recreate();
			m_Sync.Create(m_FramesInFlight, m_Swapchain.GetImageCount());
			m_RenderPass.Recreate(m_Swapchain);
			m_Framebuffers.Recreate(m_Swapchain, m_RenderPass);
			m_Pipeline.Recreate(m_RenderPass);
		}
		else if (l_PresentResult != VK_SUCCESS)
		{
			TR_CORE_CRITICAL("vkQueuePresentKHR failed with {}", (int)l_PresentResult);

			std::abort();
		}

		m_FrameResources.AdvanceFrame();
		m_FrameBegun = false;
	}
}