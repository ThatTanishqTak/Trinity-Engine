#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"

#include "Trinity/Renderer/Vulkan/VulkanShaderInterop.h"
#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Platform/Window.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>

namespace Trinity
{
	VulkanImageTransitionState VulkanRenderer::BuildTransitionState(ImageTransitionPreset preset)
	{
		if (preset == ImageTransitionPreset::Present)
		{
			return g_PresentImageState;
		}

		if (preset == ImageTransitionPreset::ColorAttachmentWrite)
		{
			return g_ColorAttachmentWriteImageState;
		}

		if (preset == ImageTransitionPreset::DepthAttachmentWrite)
		{
			return g_DepthAttachmentWriteImageState;
		}

		if (preset == ImageTransitionPreset::ShaderReadOnly)
		{
			return g_ShaderReadOnlyImageState;
		}

		if (preset == ImageTransitionPreset::TransferSource)
		{
			return g_TransferSourceImageState;
		}

		if (preset == ImageTransitionPreset::TransferDestination)
		{
			return g_TransferDestinationImageState;
		}

		return g_GeneralComputeReadWriteImageState;
	}

	VkImageSubresourceRange VulkanRenderer::BuildColorSubresourceRange()
	{
		VkImageSubresourceRange l_SubresourceRange{};
		l_SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_SubresourceRange.baseMipLevel = 0;
		l_SubresourceRange.levelCount = 1;
		l_SubresourceRange.baseArrayLayer = 0;
		l_SubresourceRange.layerCount = 1;

		return l_SubresourceRange;
	}

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
		m_ResourceStateTracker.Reset();
		m_ImGuiLayer = nullptr;
		m_ImGuiVulkanInitialized = false;
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
		m_ResourceStateTracker.Reset();
		m_Swapchain.Shutdown();
		m_Device.Shutdown();
		m_Context.Shutdown();
		m_ImGuiLayer = nullptr;
		m_ImGuiVulkanInitialized = false;
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

		const std::vector<VkImage> l_OldSwapchainImages = m_Swapchain.GetImages();

		m_Swapchain.Recreate(width, height);
		m_Sync.OnSwapchainRecreated(m_Swapchain.GetImageCount());
		m_Pipeline.Recreate(m_Swapchain.GetImageFormat());

		if (m_ImGuiVulkanInitialized && m_ImGuiLayer != nullptr)
		{
			m_ImGuiLayer->OnSwapchainRecreated(2u, m_Swapchain.GetImageCount());
		}

		for (VkImage it_Image : l_OldSwapchainImages)
		{
			m_ResourceStateTracker.ForgetImage(it_Image);
		}
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
		const VulkanImageTransitionState l_ColorAttachmentWriteState = BuildTransitionState(ImageTransitionPreset::ColorAttachmentWrite);

		const VkImageSubresourceRange l_ColorSubresourceRange = BuildColorSubresourceRange();

		TransitionImageResource(l_CommandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], l_ColorSubresourceRange, l_ColorAttachmentWriteState);

		VkClearValue l_ClearColor{};
		l_ClearColor.color.float32[0] = 0.01f;
		l_ClearColor.color.float32[1] = 0.01f;
		l_ClearColor.color.float32[2] = 0.01f;
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
		const VulkanImageTransitionState l_PresentState = BuildTransitionState(ImageTransitionPreset::Present);

		const VkImageSubresourceRange l_ColorSubresourceRange = BuildColorSubresourceRange();

		if (m_ImGuiLayer != nullptr)
		{
			m_ImGuiLayer->EndFrame(l_CommandBuffer);
		}

		vkCmdEndRendering(l_CommandBuffer);

		TransitionImageResource(l_CommandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], l_ColorSubresourceRange, l_PresentState);

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

	void VulkanRenderer::RenderImGui(ImGuiLayer& imGuiLayer)
	{
		m_ImGuiLayer = &imGuiLayer;

		if (m_ImGuiVulkanInitialized)
		{
			return;
		}

		imGuiLayer.InitializeVulkan(m_Context.GetInstance(), m_Device.GetPhysicalDevice(), m_Device.GetDevice(), m_Device.GetGraphicsQueueFamilyIndex(),
			m_Device.GetGraphicsQueue(), m_Swapchain.GetImageFormat(), m_Swapchain.GetImageCount(), 2u);

		m_ImGuiVulkanInitialized = true;
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

		SimplePushConstants l_PushConstants{};
		const glm::mat4 l_ModelMatrix = glm::translate(glm::mat4(1.0f), position);
		l_PushConstants.ModelViewProjection = viewProjection * l_ModelMatrix;
		l_PushConstants.Color = color;

		vkCmdPushConstants(l_CommandBuffer, m_Pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstants), &l_PushConstants);

		vkCmdDrawIndexed(l_CommandBuffer, a_GPUPrimitive.VulkanIB->GetIndexCount(), 1, 0, 0, 0);
	}

	void VulkanRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection)
	{
		DrawMesh(primitive, position, color, projection * view);
	}

	void VulkanRenderer::TransitionImageResource(VkCommandBuffer commandBuffer, VkImage image, const VkImageSubresourceRange& subresourceRange, const VulkanImageTransitionState& newState)
	{
		m_ResourceStateTracker.TransitionImageResource(commandBuffer, image, subresourceRange, newState);
	}
}