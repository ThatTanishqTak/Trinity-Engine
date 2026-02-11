#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"

#include "Trinity/Platform/Window.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>

namespace Trinity
{
	VulkanRenderer::ImageResourceState VulkanRenderer::BuildImageResourceState(VkImageLayout layout)
	{
		VulkanImageTransitionState l_TransitionState{};

		if (layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			l_TransitionState = g_ColorAttachmentWriteImageState;
		}
		else if (layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			l_TransitionState = g_TransferSourceImageState;
		}
		else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			l_TransitionState = g_TransferDestinationImageState;
		}
		else if (layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			l_TransitionState = g_ShaderReadOnlyImageState;
		}
		else if (layout == VK_IMAGE_LAYOUT_GENERAL)
		{
			l_TransitionState = g_GeneralComputeReadWriteImageState;
		}
		else if (layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			l_TransitionState = g_PresentImageState;
		}
		else
		{
			l_TransitionState = CreateVulkanImageTransitionState(layout, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT);
		}

		return BuildImageResourceState(l_TransitionState);
	}

	VulkanRenderer::ImageResourceState VulkanRenderer::BuildImageResourceState(const VulkanImageTransitionState& transitionState)
	{
		ImageResourceState l_ImageResourceState{};
		l_ImageResourceState.m_Layout = transitionState.m_Layout;
		l_ImageResourceState.m_Stages = transitionState.m_StageMask;
		l_ImageResourceState.m_Access = transitionState.m_AccessMask;
		l_ImageResourceState.m_QueueFamilyIndex = transitionState.m_QueueFamilyIndex;

		return l_ImageResourceState;
	}

	VulkanRenderer::SwapchainImageState VulkanRenderer::BuildSwapchainImageState()
	{
		SwapchainImageState l_SwapchainImageState{};
		l_SwapchainImageState.m_ColorAspectState = BuildImageResourceState(VK_IMAGE_LAYOUT_UNDEFINED);
		l_SwapchainImageState.m_DepthAspectState = BuildImageResourceState(VK_IMAGE_LAYOUT_UNDEFINED);

		return l_SwapchainImageState;
	}

	struct PushConstants
	{
		glm::mat4 ModelViewProjection;
		glm::vec4 Color;
	};

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
		if (!m_Window)
		{
			TR_CORE_CRITICAL("VulkanRenderer::Initialize called without a window");

			std::abort();
		}

		const NativeWindowHandle l_NativeWindowHandle = m_Window->GetNativeHandle();

#ifdef _WIN32
		if (l_NativeWindowHandle.WindowType != NativeWindowHandle::Type::Win32)
		{
			TR_CORE_CRITICAL("VulkanRenderer requires a Win32 native window handle when built for Windows");

			std::abort();
		}
#else
		TR_CORE_CRITICAL("VulkanRenderer is only implemented for Win32 native window handles in this build");

		std::abort();
#endif

		m_Context.Initialize(l_NativeWindowHandle);
		m_Device.Initialize(m_Context);
		m_Swapchain.Initialize(m_Context, m_Device, m_Window->GetWidth(), m_Window->GetHeight());
		m_Sync.Initialize(m_Context, m_Device, m_FramesInFlight, m_Swapchain.GetImageCount());
		m_Command.Initialize(m_Context, m_Device, m_FramesInFlight);
		m_Pipeline.Initialize(m_Context, m_Device, m_Swapchain.GetImageFormat(), m_VertexShaderPath, m_FragmentShaderPath);
		m_SwapchainImageStates.assign(m_Swapchain.GetImageCount(), BuildSwapchainImageState());
	}

	void VulkanRenderer::Shutdown()
	{
		if (m_Device.GetDevice() != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_Device.GetDevice());
		}

		for (auto& it_Primitive : m_Primitives)
		{
			it_Primitive.VulkanVB.reset();
			it_Primitive.VulkanIB.reset();
		}

		m_Pipeline.Shutdown();
		m_Command.Shutdown();
		m_Sync.Shutdown();
		m_Swapchain.Shutdown();
		m_Device.Shutdown();
		m_Context.Shutdown();
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
		m_SwapchainImageStates.assign(m_Swapchain.GetImageCount(), BuildSwapchainImageState());
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

		VkResult l_Acquire = m_Swapchain.AcquireNextImageIndex(l_ImageAvailable, m_CurrentImageIndex);
		if (l_Acquire == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain(m_Window->GetWidth(), m_Window->GetHeight());

			return;
		}

		if (l_Acquire != VK_SUCCESS && l_Acquire != VK_SUBOPTIMAL_KHR)
		{
			TR_CORE_CRITICAL("vkAcquireNextImageKHR failed (VkResult = {})", (int)l_Acquire);

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

		const ImageResourceState l_ColorAttachmentWriteState = BuildImageResourceState(g_ColorAttachmentWriteImageState);

		TransitionSwapchainImage(l_CommandBuffer, m_CurrentImageIndex, l_ColorAttachmentWriteState);

		VkClearValue l_ClearColor{};
		l_ClearColor.color.float32[0] = 0.05f;
		l_ClearColor.color.float32[1] = 0.05f;
		l_ClearColor.color.float32[2] = 0.05f;
		l_ClearColor.color.float32[3] = 1.0f;

		VkRenderingAttachmentInfo l_ColorAttachmentInfo{};
		l_ColorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		l_ColorAttachmentInfo.imageView = m_Swapchain.GetImageViews()[m_CurrentImageIndex];
		l_ColorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		l_ColorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_ColorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_ColorAttachmentInfo.clearValue = l_ClearColor;

		VkRenderingInfo l_RenderingInfo{};
		l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		l_RenderingInfo.renderArea.offset = { 0, 0 };
		l_RenderingInfo.renderArea.extent = m_Swapchain.GetExtent();
		l_RenderingInfo.layerCount = 1;
		l_RenderingInfo.colorAttachmentCount = 1;
		l_RenderingInfo.pColorAttachments = &l_ColorAttachmentInfo;

		vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);

		const VkExtent2D l_Extent = m_Swapchain.GetExtent();

		VkViewport l_Viewport{};
		l_Viewport.x = 0.0f;
		l_Viewport.y = 0.0f;
		l_Viewport.width = (float)l_Extent.width;
		l_Viewport.height = (float)l_Extent.height;
		l_Viewport.minDepth = 0.0f;
		l_Viewport.maxDepth = 1.0f;

		VkRect2D l_Scissor{};
		l_Scissor.offset = { 0, 0 };
		l_Scissor.extent = l_Extent;

		vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);
		vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);

		m_Pipeline.Bind(l_CommandBuffer);

		m_FrameBegun = true;
	}

	void VulkanRenderer::EndFrame()
	{
		if (!m_FrameBegun)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		const ImageResourceState l_PresentState = BuildImageResourceState(g_PresentImageState);

		vkCmdEndRendering(l_CommandBuffer);

		TransitionSwapchainImage(l_CommandBuffer, m_CurrentImageIndex, l_PresentState);

		m_Command.End(m_CurrentFrameIndex);

		const VkSemaphore l_ImageAvailable = m_Sync.GetImageAvailableSemaphore(m_CurrentFrameIndex);
		const VkSemaphore l_RenderFinished = m_Sync.GetRenderFinishedSemaphore(m_CurrentImageIndex);
		const VkFence l_InFlightFence = m_Sync.GetInFlightFence(m_CurrentFrameIndex);

		VkPipelineStageFlags l_WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo l_Submit{};
		l_Submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		l_Submit.waitSemaphoreCount = 1;
		l_Submit.pWaitSemaphores = &l_ImageAvailable;
		l_Submit.pWaitDstStageMask = &l_WaitStage;
		l_Submit.commandBufferCount = 1;
		l_Submit.pCommandBuffers = &l_CommandBuffer;
		l_Submit.signalSemaphoreCount = 1;
		l_Submit.pSignalSemaphores = &l_RenderFinished;

		Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &l_Submit, l_InFlightFence), "Failed vkQueueSubmit");

		VkResult l_Present = m_Swapchain.Present(m_Device.GetPresentQueue(), l_RenderFinished, m_CurrentImageIndex);
		if (l_Present == VK_ERROR_OUT_OF_DATE_KHR || l_Present == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapchain(m_Window->GetWidth(), m_Window->GetHeight());
		}
		else if (l_Present != VK_SUCCESS)
		{
			TR_CORE_CRITICAL("vkQueuePresentKHR failed (VkResult = {})", (int)l_Present);

			std::abort();
		}

		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_FramesInFlight;
		m_FrameBegun = false;
	}

	void VulkanRenderer::EnsurePrimitiveUploaded(Geometry::PrimitiveType primitive)
	{
		const size_t l_Index = (size_t)primitive;
		if (l_Index >= m_Primitives.size())
		{
			TR_CORE_CRITICAL("PrimitiveType out of range");

			std::abort();
		}

		auto& a_GPUPrimitive = m_Primitives[l_Index];
		if (a_GPUPrimitive.VulkanVB && a_GPUPrimitive.VulkanIB)
		{
			return;
		}

		const auto& a_Mesh = Geometry::GetPrimitive(primitive);

		a_GPUPrimitive.VulkanVB = std::make_unique<VulkanVertexBuffer>(m_Context, m_Device, (uint64_t)(a_Mesh.Vertices.size() * sizeof(Geometry::Vertex)),
			(uint32_t)sizeof(Geometry::Vertex), BufferMemoryUsage::GPUOnly, a_Mesh.Vertices.data());

		a_GPUPrimitive.VulkanIB = std::make_unique<VulkanIndexBuffer>(m_Context, m_Device, (uint64_t)(a_Mesh.Indices.size() * sizeof(uint32_t)),
			(uint32_t)a_Mesh.Indices.size(), IndexType::UInt32, BufferMemoryUsage::GPUOnly, a_Mesh.Indices.data());
	}

	void VulkanRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color)
	{
		DrawMesh(primitive, position, color, glm::mat4(1.0f));
	}

	void VulkanRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& viewProjection)
	{
		if (!m_FrameBegun)
		{
			TR_CORE_CRITICAL("DrawMesh called outside BeginFrame/EndFrame");

			std::abort();
		}

		EnsurePrimitiveUploaded(primitive);

		const size_t l_Index = (size_t)primitive;
		auto& a_GPUPrimitive = m_Primitives[l_Index];

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

		VkBuffer l_VertexBuffer = a_GPUPrimitive.VulkanVB->GetVkBuffer();
		VkDeviceSize l_Offsets[] = { 0 };
		vkCmdBindVertexBuffers(l_CommandBuffer, 0, 1, &l_VertexBuffer, l_Offsets);

		VkBuffer l_IndexBuffer = a_GPUPrimitive.VulkanIB->GetVkBuffer();
		vkCmdBindIndexBuffer(l_CommandBuffer, l_IndexBuffer, 0, ToVkIndexType(a_GPUPrimitive.VulkanIB->GetIndexType()));

		PushConstants l_PushConstants{};
		const glm::mat4 l_ModelMatrix = glm::translate(glm::mat4(1.0f), position);
		l_PushConstants.ModelViewProjection = viewProjection * l_ModelMatrix;
		l_PushConstants.Color = color;

		vkCmdPushConstants(l_CommandBuffer, m_Pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &l_PushConstants);

		vkCmdDrawIndexed(l_CommandBuffer, a_GPUPrimitive.VulkanIB->GetIndexCount(), 1, 0, 0, 0);
	}

	void VulkanRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection)
	{
		DrawMesh(primitive, position, color, projection * view);
	}

	void VulkanRenderer::TransitionSwapchainImage(VkCommandBuffer commandBuffer, uint32_t imageIndex, const ImageResourceState& newColorAspectState)
	{
		if (imageIndex >= m_SwapchainImageStates.size())
		{
			TR_CORE_CRITICAL("Swapchain image index out of range in TransitionSwapchainImage");

			std::abort();
		}

		ImageResourceState& l_ColorAspectState = m_SwapchainImageStates[imageIndex].m_ColorAspectState;
		if (l_ColorAspectState.m_Layout == newColorAspectState.m_Layout
			&& l_ColorAspectState.m_Stages == newColorAspectState.m_Stages
			&& l_ColorAspectState.m_Access == newColorAspectState.m_Access
			&& l_ColorAspectState.m_QueueFamilyIndex == newColorAspectState.m_QueueFamilyIndex)
		{
			return;
		}

		VkImageSubresourceRange l_SubresourceRange{};
		l_SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_SubresourceRange.baseMipLevel = 0;
		l_SubresourceRange.levelCount = 1;
		l_SubresourceRange.baseArrayLayer = 0;
		l_SubresourceRange.layerCount = 1;

		const VulkanImageTransitionState l_OldState = CreateVulkanImageTransitionState(l_ColorAspectState.m_Layout, l_ColorAspectState.m_Stages, l_ColorAspectState.m_Access, l_ColorAspectState.m_QueueFamilyIndex);
		const VulkanImageTransitionState l_NewState = CreateVulkanImageTransitionState(newColorAspectState.m_Layout, newColorAspectState.m_Stages, newColorAspectState.m_Access, newColorAspectState.m_QueueFamilyIndex);

		TransitionImage(commandBuffer, m_Swapchain.GetImages()[imageIndex], l_OldState, l_NewState, l_SubresourceRange);

		l_ColorAspectState = newColorAspectState;
	}
}