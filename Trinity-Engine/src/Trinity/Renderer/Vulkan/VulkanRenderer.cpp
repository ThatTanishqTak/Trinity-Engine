#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"

#include "Trinity/Platform/Window.h"
#include "Trinity/Utilities/Utilities.h"

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
		if (m_Window == nullptr)
		{
			TR_CORE_CRITICAL("VulkanRenderer::Initialize called without a Window");

			std::abort();
		}

		TR_CORE_TRACE("Initializing vulkan renderer");

		// Core Vulkan
		m_Instance.Initialize();
		m_Surface.Initialize(m_Instance, *m_Window);
		m_Device.Initialize(m_Instance.GetInstance(), m_Surface.GetSurface(), m_Instance.GetAllocator());

		// Context (single source of truth)
		m_Context = VulkanContext::Initialize(m_Instance, m_Surface, m_Device);

		// Swapchain chain
		m_Swapchain.Initialize(m_Context, m_Window->IsVSync());
		m_RenderPass.Initialize(m_Context, m_Swapchain);
		m_Framebuffers.Initialize(m_Context, m_Swapchain, m_RenderPass);

		// Per-frame bookkeeping
		m_FrameResources.Initialize(m_FramesInFlight);

		// Descriptors + pipeline
		m_Descriptors.Initialize(m_Context, m_FramesInFlight);

		VulkanPipeline::GraphicsDescription l_PipelineDesc{};
		l_PipelineDesc.EnableDepthTest = true;
		l_PipelineDesc.EnableDepthWrite = true;
		l_PipelineDesc.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		l_PipelineDesc.CullMode = VK_CULL_MODE_BACK_BIT;
		l_PipelineDesc.FrontFace = VK_FRONT_FACE_CLOCKWISE;
		l_PipelineDesc.VertexBindings.clear();
		l_PipelineDesc.VertexAttributes.clear();
		m_Pipeline.SetGraphicsDescription(l_PipelineDesc);

		m_Pipeline.Initialize(m_Context, m_RenderPass, m_Descriptors, "Assets/Shaders/Simple.vert.spv", "Assets/Shaders/Simple.frag.spv", true);

		// Commands + sync
		m_Command.Initialize(m_Context, m_FramesInFlight);
		m_Sync.Initialize(m_Context, m_FramesInFlight, m_Swapchain.GetImageCount());

		std::vector<Geometry::Vertex> l_Vertices =
		{
			{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.0f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }
		};
		std::vector<uint32_t> l_Indices = { 0, 1, 2 };
		m_Mesh.Upload(m_Device, m_Command, l_Vertices, l_Indices);

		TR_CORE_TRACE("Vulkan renderer initialized");
	}

	void VulkanRenderer::Shutdown()
	{
		if (m_Context.Device != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_Context.Device);
		}

		m_Sync.Shutdown();
		m_Command.Shutdown();
		m_Mesh.Destroy();
		m_Pipeline.Shutdown();
		m_Descriptors.Shutdown();
		m_FrameResources.Shutdown();
		m_Framebuffers.Shutdown();
		m_RenderPass.Shutdown();
		m_Swapchain.Shutdown();
		m_Device.Shutdown();
		m_Surface.Shutdown(m_Instance);
		m_Instance.Shutdown();

		m_Context = {};
	}

	void VulkanRenderer::Resize(uint32_t width, uint32_t height)
	{
		m_PendingWidth = width;
		m_PendingHeight = height;
		m_ResizePending = true;
	}

	void VulkanRenderer::BeginFrame()
	{
		m_FrameBegun = false;
		m_FrameContext.reset();

		if (m_ResizePending)
		{
			RecreateSwapchain(m_PendingWidth, m_PendingHeight);
			m_ResizePending = false;

			const VkExtent2D l_Extent = m_Swapchain.GetExtent();
			if (l_Extent.width == 0 || l_Extent.height == 0)
			{
				return; // minimized: skip rendering until we get a real size
			}
		}

		const uint32_t l_FrameIndex = m_FrameResources.GetCurrentFrameIndex();

		m_Sync.WaitForFrameFence(l_FrameIndex);

		const VkSemaphore l_ImageAvailableSemaphore = m_Sync.GetImageAvailableSemaphore(l_FrameIndex);

		uint32_t l_ImageIndex = 0;
		VkResult l_AcquireResult = m_Swapchain.AcquireNextImage(l_ImageAvailableSemaphore, l_ImageIndex);
		if (l_AcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain(m_PendingWidth, m_PendingHeight);

			return;
		}

		if (l_AcquireResult != VK_SUCCESS && l_AcquireResult != VK_SUBOPTIMAL_KHR)
		{
			TR_CORE_CRITICAL("vkAcquireNextImageKHR failed with {}", (int)l_AcquireResult);

			std::abort();
		}

		const VkFence l_FrameFence = m_Sync.GetInFlightFence(l_FrameIndex);
		const VkFence l_ImageFence = m_Sync.GetImageInFlightFence(l_ImageIndex);

		if (l_ImageFence != VK_NULL_HANDLE && l_ImageFence != l_FrameFence)
		{
			Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Context.Device, 1, &l_ImageFence, VK_TRUE, UINT64_MAX), "Failed vkWaitForFences (image in flight)");
		}

		m_Sync.ResetFrameFence(l_FrameIndex);
		m_Sync.SetImageInFlightFence(l_ImageIndex, l_FrameFence);

		VkCommandBuffer l_CommandBuffer = m_Command.BeginFrame(l_FrameIndex);

		const VkSemaphore l_RenderFinishedSemaphore = m_Sync.GetRenderFinishedSemaphore(l_ImageIndex);

		FrameContext l_FrameContext{};
		l_FrameContext.FrameIndex = l_FrameIndex;
		l_FrameContext.ImageIndex = l_ImageIndex;
		l_FrameContext.CommandBuffer = l_CommandBuffer;
		l_FrameContext.ImageAvailableSemaphore = l_ImageAvailableSemaphore;
		l_FrameContext.RenderFinishedSemaphore = l_RenderFinishedSemaphore;
		l_FrameContext.InFlightFence = l_FrameFence;

		m_FrameContext = l_FrameContext;
		m_FrameBegun = true;

		VkClearValue l_ClearValues[2]{};

		// Color clear
		l_ClearValues[0].color.float32[0] = 0.01f;
		l_ClearValues[0].color.float32[1] = 0.01f;
		l_ClearValues[0].color.float32[2] = 0.01f;
		l_ClearValues[0].color.float32[3] = 1.0f;

		// Depth clear
		l_ClearValues[1].depthStencil.depth = 1.0f;
		l_ClearValues[1].depthStencil.stencil = 0;

		VkRenderPassBeginInfo l_RPBegin{};
		l_RPBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		l_RPBegin.renderPass = m_RenderPass.GetRenderPass();
		l_RPBegin.framebuffer = m_Framebuffers.GetFramebuffer(l_ImageIndex);
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

		VkBuffer l_VertexBuffer = m_Mesh.VertexBuffer.GetBuffer();
		VkDeviceSize l_VertexOffsets[] = { 0 };
		vkCmdBindVertexBuffers(l_CommandBuffer, 0, 1, &l_VertexBuffer, l_VertexOffsets);
		vkCmdBindIndexBuffer(l_CommandBuffer, m_Mesh.IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(l_CommandBuffer, m_Mesh.IndexCount, 1, 0, 0, 0);
	}

	void VulkanRenderer::EndFrame()
	{
		if (!m_FrameBegun || !m_FrameContext.has_value())
		{
			return;
		}

		const FrameContext& l_FrameContext = m_FrameContext.value();
		VkCommandBuffer l_CommandBuffer = l_FrameContext.CommandBuffer;

		vkCmdEndRenderPass(l_CommandBuffer);
		m_Command.EndFrame(l_FrameContext.FrameIndex);

		const VkSemaphore l_ImageAvailableSemaphore = l_FrameContext.ImageAvailableSemaphore;
		const VkSemaphore l_RenderFinishedSemaphore = l_FrameContext.RenderFinishedSemaphore;
		const VkFence l_InFlightFence = l_FrameContext.InFlightFence;

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

		Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_Context.Queues.GraphicsQueue, 1, &l_SubmitInfo, l_InFlightFence), "Failed vkQueueSubmit");

		VkResult l_PresentResult = m_Swapchain.Present(m_Context.Queues.PresentQueue, l_FrameContext.ImageIndex, l_RenderFinishedSemaphore);

		if (l_PresentResult == VK_ERROR_OUT_OF_DATE_KHR || l_PresentResult == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapchain(m_PendingWidth, m_PendingHeight);
		}
		else if (l_PresentResult != VK_SUCCESS)
		{
			TR_CORE_CRITICAL("vkQueuePresentKHR failed with {}", (int)l_PresentResult);

			std::abort();
		}

		m_FrameResources.AdvanceFrame();
		m_FrameBegun = false;
		m_FrameContext.reset();
	}

	void VulkanRenderer::RecreateSwapchain(uint32_t preferredWidth, uint32_t preferredHeight)
	{
		if (m_Context.Device == VK_NULL_HANDLE)
		{
			return;
		}

		vkDeviceWaitIdle(m_Context.Device);

		const bool l_Recreated = m_Swapchain.Recreate(preferredWidth, preferredHeight);
		if (!l_Recreated)
		{
			return;
		}

		// Swapchain-dependent
		m_Sync.RecreateForSwapchain(m_Swapchain.GetImageCount());

		m_RenderPass.Recreate(m_Swapchain);
		m_Framebuffers.Recreate(m_Swapchain, m_RenderPass);
		m_Pipeline.Recreate(m_RenderPass);

		m_FrameResources.Reset();


		m_FrameContext.reset();
		m_FrameBegun = false;
	}
}