#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"

#include "Trinity/Renderer/Vulkan/VulkanAllocator.h"
#include "Trinity/Renderer/Vulkan/VulkanShaderInterop.h"
#include "Trinity/Renderer/Vulkan/VulkanTexture.h"
#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Platform/Window.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <cstring>
#include <array>

namespace Trinity
{
	VulkanImageTransitionState VulkanRenderer::BuildTransitionState(ImageTransitionPreset preset)
	{
		switch (preset)
		{
		case ImageTransitionPreset::Present:
			return g_PresentImageState;
		case ImageTransitionPreset::ColorAttachmentWrite:
			return g_ColorAttachmentWriteImageState;
		case ImageTransitionPreset::DepthAttachmentWrite:
			return g_DepthAttachmentWriteImageState;
		case ImageTransitionPreset::ShaderReadOnly:
			return g_ShaderReadOnlyImageState;
		case ImageTransitionPreset::TransferSource:
			return g_TransferSourceImageState;
		case ImageTransitionPreset::TransferDestination:
			return g_TransferDestinationImageState;
		case ImageTransitionPreset::GeneralComputeReadWrite:
			return g_GeneralComputeReadWriteImageState;
		case ImageTransitionPreset::DepthShaderReadOnly:
			return g_DepthShaderReadOnlyImageState;
		default:
			TR_CORE_CRITICAL("BuildTransitionState: unhandled ImageTransitionPreset");
			std::abort();
		}
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

	VkImageSubresourceRange VulkanRenderer::BuildDepthSubresourceRange()
	{
		VkImageSubresourceRange l_SubresourceRange{};
		l_SubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		l_SubresourceRange.baseMipLevel = 0;
		l_SubresourceRange.levelCount = 1;
		l_SubresourceRange.baseArrayLayer = 0;
		l_SubresourceRange.layerCount = 1;

		return l_SubresourceRange;
	}

	VulkanRenderer::VulkanRenderer() : Renderer(RendererAPI::VULKAN)
	{

	}

	VulkanRenderer::VulkanRenderer(const Configuration& configuration) : Renderer(RendererAPI::VULKAN), m_Configuration(configuration)
	{

	}

	void VulkanRenderer::SetWindow(Window& window)
	{
		m_Window = &window;
	}

	void VulkanRenderer::SetConfiguration(const Configuration& configuration)
	{
		m_Configuration = configuration;
	}

	void VulkanRenderer::Initialize()
	{
		if (!m_Window)
		{
			TR_CORE_CRITICAL("VulkanRenderer::Initialize called without a window");
			std::abort();
		}

		const NativeWindowHandle l_NativeWindowHandle = m_Window->GetNativeHandle();

		m_Context.Initialize(l_NativeWindowHandle);
		m_Device.Initialize(m_Context);
		m_Allocator.Initialize(m_Context, m_Device);
		m_SceneViewportDepthFormat = Utilities::VulkanUtilities::FindDepthFormat(m_Device.GetPhysicalDevice());
		m_Swapchain.Initialize(m_Context, m_Device, m_Window->GetWidth(), m_Window->GetHeight(), m_Configuration.m_ColorOutputPolicy);
		m_Sync.Initialize(m_Context, m_Device, m_FramesInFlight, m_Swapchain.GetImageCount());
		m_Command.Initialize(m_Context, m_Device, m_FramesInFlight);
		m_UploadContext.Initialize(m_Context, m_Device);

		m_ShaderLibrary.Load(m_Configuration.m_VertexShaderName, m_Configuration.m_VertexShaderPath, ShaderStage::Vertex);
		m_ShaderLibrary.Load(m_Configuration.m_FragmentShaderName, m_Configuration.m_FragmentShaderPath, ShaderStage::Fragment);

		const std::vector<uint32_t>& l_VertexSpirV = *m_ShaderLibrary.GetSpirV(m_Configuration.m_VertexShaderName);
		const std::vector<uint32_t>& l_FragmentSpirV = *m_ShaderLibrary.GetSpirV(m_Configuration.m_FragmentShaderName);

		m_Pipeline.Initialize(m_Context, m_Device, m_Swapchain.GetImageFormat(), m_SceneViewportDepthFormat, l_VertexSpirV, l_FragmentSpirV);
		m_ResourceStateTracker.Reset();
		m_ImGuiLayer = nullptr;
		m_ImGuiVulkanInitialized = false;
		m_SceneViewportHandle = nullptr;

		InitializeDeferredResources();
	}

	void VulkanRenderer::Shutdown()
	{
		if (m_Device.GetDevice() != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_Device.GetDevice());
		}

		DestroySceneViewportTarget();
		ShutdownDeferredResources();

		for (auto& it_Primitive : m_Primitives)
		{
			it_Primitive.VulkanVB.reset();
			it_Primitive.VulkanIB.reset();
		}

		m_UploadContext.Shutdown();
		m_Pipeline.Shutdown();
		m_ShaderLibrary.Clear();
		m_Command.Shutdown();
		m_Sync.Shutdown();
		m_ResourceStateTracker.Reset();
		m_Swapchain.Shutdown();
		m_Allocator.Shutdown();
		m_Device.Shutdown();
		m_Context.Shutdown();
		m_ImGuiLayer = nullptr;
		m_ImGuiVulkanInitialized = false;
	}

	void VulkanRenderer::Resize(uint32_t width, uint32_t height)
	{
		RecreateSwapchain(width, height);
	}

	void VulkanRenderer::SetSceneViewportSize(uint32_t width, uint32_t height)
	{
		if (m_SceneViewportWidth == width && m_SceneViewportHeight == height)
		{
			return;
		}

		m_PendingWidth = width;
		m_PendingHeight = height;
		m_PendingViewportRecreate = true;
	}

	void* VulkanRenderer::GetSceneViewportHandle() const
	{
		return m_SceneViewportHandle;
	}

	void VulkanRenderer::RecreateSwapchain(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
		{
			return;
		}

		if (m_Device.GetDevice() == VK_NULL_HANDLE)
		{
			return;
		}

		vkDeviceWaitIdle(m_Device.GetDevice());

		const std::vector<VkImage> l_OldSwapchainImages = m_Swapchain.GetImages();

		m_Swapchain.Recreate(width, height);
		m_Sync.OnSwapchainRecreated(m_Swapchain.GetImageCount());
		m_Pipeline.Recreate(m_Swapchain.GetImageFormat());

		if (m_ImGuiLayer != nullptr)
		{
			m_ImGuiLayer->OnSwapchainRecreated(m_Swapchain.GetImageCount(), m_Swapchain.GetImageFormat());
		}

		for (VkImage it_Image : l_OldSwapchainImages)
		{
			m_ResourceStateTracker.ForgetImage(it_Image);
		}

		RecreateSceneViewportTarget();
	}

	void VulkanRenderer::RecreateSceneViewportTarget()
	{
		DestroySceneViewportTarget();

		if (m_SceneViewportWidth == 0 || m_SceneViewportHeight == 0)
		{
			return;
		}

		VkImageCreateInfo l_ImageCreateInfo{};
		l_ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		l_ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		l_ImageCreateInfo.extent.width = m_SceneViewportWidth;
		l_ImageCreateInfo.extent.height = m_SceneViewportHeight;
		l_ImageCreateInfo.extent.depth = 1;
		l_ImageCreateInfo.mipLevels = 1;
		l_ImageCreateInfo.arrayLayers = 1;
		l_ImageCreateInfo.format = m_Swapchain.GetImageFormat();
		l_ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		l_ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		l_ImageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		l_ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		l_ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		m_Allocator.CreateImage(l_ImageCreateInfo, m_SceneViewportImage, m_SceneViewportImageAllocation);

		VkImageViewCreateInfo l_ViewCreateInfo{};
		l_ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		l_ViewCreateInfo.image = m_SceneViewportImage;
		l_ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		l_ViewCreateInfo.format = m_Swapchain.GetImageFormat();
		l_ViewCreateInfo.subresourceRange = BuildColorSubresourceRange();

		Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device.GetDevice(), &l_ViewCreateInfo, m_Context.GetAllocator(), &m_SceneViewportImageView), "Failed vkCreateImageView");

		VkSamplerCreateInfo l_SamplerCreateInfo{};
		l_SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		l_SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		l_SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		l_SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		l_SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		l_SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		l_SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		l_SamplerCreateInfo.minLod = 0.0f;
		l_SamplerCreateInfo.maxLod = 0.0f;
		l_SamplerCreateInfo.maxAnisotropy = 1.0f;

		Utilities::VulkanUtilities::VKCheck(vkCreateSampler(m_Device.GetDevice(), &l_SamplerCreateInfo, m_Context.GetAllocator(), &m_SceneViewportSampler), "Failed vkCreateSampler");

		m_SceneViewportHandle = ImGui_ImplVulkan_AddTexture(m_SceneViewportSampler, m_SceneViewportImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		if (m_SceneViewportDepthFormat == VK_FORMAT_UNDEFINED)
		{
			TR_CORE_CRITICAL("Scene viewport depth format is undefined");
			std::abort();
		}

		VkImageCreateInfo l_DepthImageCreateInfo{};
		l_DepthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		l_DepthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		l_DepthImageCreateInfo.extent.width = m_SceneViewportWidth;
		l_DepthImageCreateInfo.extent.height = m_SceneViewportHeight;
		l_DepthImageCreateInfo.extent.depth = 1;
		l_DepthImageCreateInfo.mipLevels = 1;
		l_DepthImageCreateInfo.arrayLayers = 1;
		l_DepthImageCreateInfo.format = m_SceneViewportDepthFormat;
		l_DepthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		l_DepthImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		l_DepthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		l_DepthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		l_DepthImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		m_Allocator.CreateImage(l_DepthImageCreateInfo, m_SceneViewportDepthImage, m_SceneViewportDepthImageAllocation);

		VkImageViewCreateInfo l_DepthViewCreateInfo{};
		l_DepthViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		l_DepthViewCreateInfo.image = m_SceneViewportDepthImage;
		l_DepthViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		l_DepthViewCreateInfo.format = m_SceneViewportDepthFormat;
		l_DepthViewCreateInfo.subresourceRange = BuildDepthSubresourceRange();

		Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device.GetDevice(), &l_DepthViewCreateInfo, m_Context.GetAllocator(), &m_SceneViewportDepthImageView), "Failed vkCreateImageView");

		RecreateDeferredResources();
	}

	void VulkanRenderer::DestroySceneViewportTarget()
	{
		if (m_SceneViewportHandle != nullptr)
		{
			if (ImGui::GetCurrentContext() != nullptr)
			{
				ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)m_SceneViewportHandle);
			}

			m_SceneViewportHandle = nullptr;
		}

		if (m_SceneViewportSampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_Device.GetDevice(), m_SceneViewportSampler, m_Context.GetAllocator());
			m_SceneViewportSampler = VK_NULL_HANDLE;
		}

		if (m_SceneViewportDepthImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_Device.GetDevice(), m_SceneViewportDepthImageView, m_Context.GetAllocator());
			m_SceneViewportDepthImageView = VK_NULL_HANDLE;
		}

		if (m_SceneViewportDepthImage != VK_NULL_HANDLE)
		{
			m_ResourceStateTracker.ForgetImage(m_SceneViewportDepthImage);
			m_Allocator.DestroyImage(m_SceneViewportDepthImage, m_SceneViewportDepthImageAllocation);
			m_SceneViewportDepthImage = VK_NULL_HANDLE;
			m_SceneViewportDepthImageAllocation = VK_NULL_HANDLE;
		}

		if (m_SceneViewportImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_Device.GetDevice(), m_SceneViewportImageView, m_Context.GetAllocator());
			m_SceneViewportImageView = VK_NULL_HANDLE;
		}

		if (m_SceneViewportImage != VK_NULL_HANDLE)
		{
			m_ResourceStateTracker.ForgetImage(m_SceneViewportImage);
			m_Allocator.DestroyImage(m_SceneViewportImage, m_SceneViewportImageAllocation);
			m_SceneViewportImage = VK_NULL_HANDLE;
			m_SceneViewportImageAllocation = VK_NULL_HANDLE;
		}
	}

	void VulkanRenderer::BeginPresentPass(VkCommandBuffer commandBuffer, const VkImageSubresourceRange& colorSubresourceRange, const VulkanImageTransitionState& colorAttachmentWriteState)
	{
		if (m_PresentPassRecording)
		{
			return;
		}

		TransitionImageResource(commandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], colorSubresourceRange, colorAttachmentWriteState);

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

		vkCmdBeginRendering(commandBuffer, &l_RenderingInfo);

		m_PresentPassRecording = true;
	}

	void VulkanRenderer::BeginFrame()
	{
		if (!m_Window || m_Window->IsMinimized())
		{
			m_FrameBegun = false;

			return;
		}

		if (m_PendingViewportRecreate && m_Device.GetDevice() != VK_NULL_HANDLE)
		{
			m_SceneViewportWidth = m_PendingWidth;
			m_SceneViewportHeight = m_PendingHeight;

			for (uint32_t i = 0; i < m_FramesInFlight; ++i)
			{
				m_Sync.WaitForFrameFence(i);
			}

			RecreateSceneViewportTarget();
			m_PendingViewportRecreate = false;
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
			TR_CORE_CRITICAL("vkAcquireNextImageKHR failed (VkResult = {})", static_cast<int>(l_Acquire));

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

		m_DescriptorAllocator.BeginFrame(m_CurrentFrameIndex);

		if (!m_LightingDescriptorPools.empty() && m_LightingDescriptorPools[m_CurrentFrameIndex] != VK_NULL_HANDLE)
		{
			vkResetDescriptorPool(m_Device.GetDevice(), m_LightingDescriptorPools[m_CurrentFrameIndex], 0);
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		const VulkanImageTransitionState l_ColorAttachmentWriteState = BuildTransitionState(ImageTransitionPreset::ColorAttachmentWrite);
		const VkImageSubresourceRange l_ColorSubresourceRange = BuildColorSubresourceRange();
		const VkExtent2D l_SceneExtent{ m_SceneViewportWidth, m_SceneViewportHeight };
		const bool l_CanRecordScenePass = m_SceneViewportImage != VK_NULL_HANDLE && m_SceneViewportDepthImage != VK_NULL_HANDLE && l_SceneExtent.width > 0 && l_SceneExtent.height > 0;

		m_ScenePassRecording = false;
		m_PresentPassRecording = false;

		if (l_CanRecordScenePass)
		{
			TransitionImageResource(l_CommandBuffer, m_SceneViewportImage, l_ColorSubresourceRange, l_ColorAttachmentWriteState);

			VkClearValue l_ClearColor{};
			l_ClearColor.color.float32[0] = 0.01f;
			l_ClearColor.color.float32[1] = 0.01f;
			l_ClearColor.color.float32[2] = 0.01f;
			l_ClearColor.color.float32[3] = 1.0f;

			VkRenderingAttachmentInfo l_ColorAttachmentInfo{};
			l_ColorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			l_ColorAttachmentInfo.imageView = m_SceneViewportImageView;
			l_ColorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			l_ColorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			l_ColorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			l_ColorAttachmentInfo.clearValue = l_ClearColor;

			VkClearValue l_ClearDepth{};
			l_ClearDepth.depthStencil.depth = 1.0f;
			l_ClearDepth.depthStencil.stencil = 0;

			VkRenderingAttachmentInfo l_DepthAttachmentInfo{};
			l_DepthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			l_DepthAttachmentInfo.imageView = m_SceneViewportDepthImageView;
			l_DepthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			l_DepthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			l_DepthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			l_DepthAttachmentInfo.clearValue = l_ClearDepth;

			VkRenderingInfo l_RenderingInfo{};
			l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			l_RenderingInfo.renderArea.offset = { 0, 0 };
			l_RenderingInfo.renderArea.extent = l_SceneExtent;
			l_RenderingInfo.layerCount = 1;
			l_RenderingInfo.colorAttachmentCount = 1;
			l_RenderingInfo.pColorAttachments = &l_ColorAttachmentInfo;
			l_RenderingInfo.pDepthAttachment = &l_DepthAttachmentInfo;

			const VkImageSubresourceRange l_DepthSubresourceRange = BuildDepthSubresourceRange();
			TransitionImageResource(l_CommandBuffer, m_SceneViewportDepthImage, l_DepthSubresourceRange, BuildTransitionState(ImageTransitionPreset::DepthAttachmentWrite));

			vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);

			VkViewport l_Viewport{};
			l_Viewport.x = 0.0f;
			l_Viewport.y = 0.0f;
			l_Viewport.width = static_cast<float>(l_SceneExtent.width);
			l_Viewport.height = static_cast<float>(l_SceneExtent.height);
			l_Viewport.minDepth = 0.0f;
			l_Viewport.maxDepth = 1.0f;

			VkRect2D l_Scissor{};
			l_Scissor.offset = { 0, 0 };
			l_Scissor.extent = l_SceneExtent;

			vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);
			vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);

			m_Pipeline.Bind(l_CommandBuffer);
			ValidateSceneColorPolicy();
			m_ScenePassRecording = true;
		}

		m_FrameBegun = true;
	}

	void VulkanRenderer::EndFrame()
	{
		if (!m_FrameBegun)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		const VkImageSubresourceRange l_ColorSubresourceRange = BuildColorSubresourceRange();
		const VulkanImageTransitionState l_ColorAttachmentWriteState = BuildTransitionState(ImageTransitionPreset::ColorAttachmentWrite);
		const VulkanImageTransitionState l_PresentState = BuildTransitionState(ImageTransitionPreset::Present);

		if (m_ScenePassRecording)
		{
			vkCmdEndRendering(l_CommandBuffer);

			TransitionImageResource(l_CommandBuffer, m_SceneViewportImage, l_ColorSubresourceRange, BuildTransitionState(ImageTransitionPreset::ShaderReadOnly));
			m_ScenePassRecording = false;
		}

		BeginPresentPass(l_CommandBuffer, l_ColorSubresourceRange, l_ColorAttachmentWriteState);
		vkCmdEndRendering(l_CommandBuffer);
		m_PresentPassRecording = false;

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
			TR_CORE_CRITICAL("vkQueuePresentKHR failed (VkResult = {})", static_cast<int>(l_Present));
			std::abort();
		}

		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_FramesInFlight;
		m_FrameBegun = false;
	}

	void VulkanRenderer::RenderImGui(ImGuiLayer& imGuiLayer)
	{
		m_ImGuiLayer = &imGuiLayer;

		if (!m_FrameBegun)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		const VkImageSubresourceRange l_ColorSubresourceRange = BuildColorSubresourceRange();
		const VulkanImageTransitionState l_ColorAttachmentWriteState = BuildTransitionState(ImageTransitionPreset::ColorAttachmentWrite);

		BeginPresentPass(l_CommandBuffer, l_ColorSubresourceRange, l_ColorAttachmentWriteState);

		ImDrawData* l_DrawData = ImGui::GetDrawData();
		if (l_DrawData != nullptr)
		{
			ImGui_ImplVulkan_RenderDrawData(l_DrawData, l_CommandBuffer);
			m_ImGuiVulkanInitialized = true;
		}
	}

	void VulkanRenderer::TransitionImageResource(VkCommandBuffer commandBuffer, VkImage image, const VkImageSubresourceRange& subresourceRange, const VulkanImageTransitionState& newState)
	{
		m_ResourceStateTracker.TransitionImageResource(commandBuffer, image, subresourceRange, newState);
	}

	void VulkanRenderer::EnsurePrimitiveUploaded(Geometry::PrimitiveType primitive)
	{
		const size_t l_Index = static_cast<size_t>(primitive);
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

		a_GPUPrimitive.VulkanVB = std::make_unique<VulkanVertexBuffer>(m_Allocator, m_UploadContext, static_cast<uint64_t>(a_Mesh.Vertices.size() * sizeof(Geometry::Vertex)), static_cast<uint32_t>(sizeof(Geometry::Vertex)), BufferMemoryUsage::GPUOnly, a_Mesh.Vertices.data());
		a_GPUPrimitive.VulkanIB = std::make_unique<VulkanIndexBuffer>(m_Allocator, m_UploadContext, static_cast<uint64_t>(a_Mesh.Indices.size() * sizeof(uint32_t)), static_cast<uint32_t>(a_Mesh.Indices.size()), IndexType::UInt32, BufferMemoryUsage::GPUOnly, a_Mesh.Indices.data());
	}

	void VulkanRenderer::ValidateSceneColorPolicy() const
	{
		const VulkanSwapchain::SceneColorPolicy& l_ColorPolicy = m_Swapchain.GetSceneColorPolicy();
		const VkFormat l_SwapchainFormat = m_Swapchain.GetImageFormat();
		const bool l_IsUnormSwapchain = l_SwapchainFormat == VK_FORMAT_B8G8R8A8_UNORM || l_SwapchainFormat == VK_FORMAT_R8G8B8A8_UNORM;
		const bool l_IsSrgbSwapchain = l_SwapchainFormat == VK_FORMAT_B8G8R8A8_SRGB || l_SwapchainFormat == VK_FORMAT_R8G8B8A8_SRGB;

		if (m_Configuration.m_ColorOutputPolicy == VulkanSwapchain::ColorOutputPolicy::SDRsRGB && l_ColorPolicy.SceneInputTransfer != ColorTransferMode::None)
		{
			TR_CORE_CRITICAL("Scene color policy validation failed: sRGB swapchain plus linear scene data must keep scene input transfer at None");

			std::abort();
		}

		if (m_Configuration.m_ColorOutputPolicy == VulkanSwapchain::ColorOutputPolicy::SDRsRGB && l_IsUnormSwapchain && l_ColorPolicy.SceneOutputTransfer != ColorTransferMode::LinearToSrgb)
		{
			TR_CORE_CRITICAL("Scene color policy validation failed: UNORM swapchain requires explicit scene output conversion");

			std::abort();
		}

		if (m_Configuration.m_ColorOutputPolicy == VulkanSwapchain::ColorOutputPolicy::SDRsRGB && l_IsUnormSwapchain && l_ColorPolicy.UiOutputTransfer != ColorTransferMode::LinearToSrgb)
		{
			TR_CORE_CRITICAL("Scene color policy validation failed: UNORM swapchain requires explicit ImGui output conversion");

			std::abort();
		}

		if (m_Configuration.m_ColorOutputPolicy == VulkanSwapchain::ColorOutputPolicy::SDRsRGB && l_IsSrgbSwapchain && l_ColorPolicy.UiInputTransfer != ColorTransferMode::SrgbToLinear)
		{
			TR_CORE_CRITICAL("Scene color policy validation failed: sRGB swapchain requires ImGui input linearization on GPU path");

			std::abort();
		}
	}

	void VulkanRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection)
	{
		if (!m_FrameBegun)
		{
			TR_CORE_CRITICAL("DrawMesh called outside BeginFrame/EndFrame");

			std::abort();
		}

		if (!m_ScenePassRecording)
		{
			return;
		}

		EnsurePrimitiveUploaded(primitive);

		const size_t l_Index = static_cast<size_t>(primitive);
		auto& a_GPUPrimitive = m_Primitives[l_Index];
		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

		VkBuffer l_VertexBuffer = a_GPUPrimitive.VulkanVB->GetVkBuffer();
		VkDeviceSize l_Offsets[] = { 0 };
		vkCmdBindVertexBuffers(l_CommandBuffer, 0, 1, &l_VertexBuffer, l_Offsets);

		VkBuffer l_IndexBuffer = a_GPUPrimitive.VulkanIB->GetVkBuffer();
		vkCmdBindIndexBuffer(l_CommandBuffer, l_IndexBuffer, 0, ToVkIndexType(a_GPUPrimitive.VulkanIB->GetIndexType()));

		SimplePushConstants l_PushConstants{};
		l_PushConstants.ModelViewProjection = viewProjection * model;
		l_PushConstants.Color = color;

		const VulkanSwapchain::SceneColorPolicy& l_ColorPolicy = m_Swapchain.GetSceneColorPolicy();
		l_PushConstants.ColorInputTransfer = static_cast<uint32_t>(l_ColorPolicy.SceneInputTransfer);
		l_PushConstants.ColorOutputTransfer = static_cast<uint32_t>(l_ColorPolicy.SceneOutputTransfer);

		vkCmdPushConstants(l_CommandBuffer, m_Pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstants), &l_PushConstants);

		vkCmdDrawIndexed(l_CommandBuffer, a_GPUPrimitive.VulkanIB->GetIndexCount(), 1, 0, 0, 0);
	}

	void VulkanRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color)
	{
		DrawMesh(primitive, glm::translate(glm::mat4(1.0f), position), color, glm::mat4(1.0f));
	}

	void VulkanRenderer::DrawMesh(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection)
	{
		if (!m_FrameBegun)
		{
			TR_CORE_CRITICAL("DrawMesh called outside BeginFrame/EndFrame");
			std::abort();
		}

		if (!m_ScenePassRecording)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

		VkBuffer l_VkVertexBuffer = reinterpret_cast<VkBuffer>(vertexBuffer.GetNativeHandle());
		VkDeviceSize l_Offsets[] = { 0 };
		vkCmdBindVertexBuffers(l_CommandBuffer, 0, 1, &l_VkVertexBuffer, l_Offsets);

		VkBuffer l_VkIndexBuffer = reinterpret_cast<VkBuffer>(indexBuffer.GetNativeHandle());
		vkCmdBindIndexBuffer(l_CommandBuffer, l_VkIndexBuffer, 0, ToVkIndexType(indexBuffer.GetIndexType()));

		SimplePushConstants l_PushConstants{};
		l_PushConstants.ModelViewProjection = viewProjection * model;
		l_PushConstants.Color = color;

		const VulkanSwapchain::SceneColorPolicy& l_ColorPolicy = m_Swapchain.GetSceneColorPolicy();
		l_PushConstants.ColorInputTransfer = static_cast<uint32_t>(l_ColorPolicy.SceneInputTransfer);
		l_PushConstants.ColorOutputTransfer = static_cast<uint32_t>(l_ColorPolicy.SceneOutputTransfer);

		vkCmdPushConstants(l_CommandBuffer, m_Pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstants), &l_PushConstants);

		vkCmdDrawIndexed(l_CommandBuffer, indexCount, 1, 0, 0, 0);
	}

	void VulkanRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& viewProjection)
	{
		DrawMesh(primitive, glm::translate(glm::mat4(1.0f), position), color, viewProjection);
	}

	void VulkanRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection)
	{
		DrawMesh(primitive, glm::translate(glm::mat4(1.0f), position), color, projection * view);
	}

	void VulkanRenderer::ReloadShaders()
	{
		if (m_Device.GetDevice() == VK_NULL_HANDLE)
		{
			return;
		}

		m_ShaderLibrary.HotReload();

		const std::vector<uint32_t>* l_VertexSpirV = m_ShaderLibrary.GetSpirV(m_Configuration.m_VertexShaderName);
		const std::vector<uint32_t>* l_FragmentSpirV = m_ShaderLibrary.GetSpirV(m_Configuration.m_FragmentShaderName);

		if (!l_VertexSpirV || !l_FragmentSpirV)
		{
			TR_CORE_WARN("VulkanRenderer::ReloadShaders — one or more shaders not found in library");

			return;
		}

		vkDeviceWaitIdle(m_Device.GetDevice());
		m_Pipeline.Initialize(m_Context, m_Device, m_Swapchain.GetImageFormat(), m_SceneViewportDepthFormat, *l_VertexSpirV, *l_FragmentSpirV);

		TR_CORE_INFO("VulkanRenderer: shaders reloaded");
	}

	void VulkanRenderer::InitializeDeferredResources()
	{
		m_DescriptorAllocator.Initialize(m_Device.GetDevice(), m_Context.GetAllocator(), m_FramesInFlight, s_MaxTextureDescriptorsPerFrame);
		m_WhiteTexture.CreateSolid(m_Allocator, m_UploadContext, m_Device.GetDevice(), m_Context.GetAllocator(), 255, 255, 255, 255);

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
			Utilities::VulkanUtilities::VKCheck(vkCreateSampler(m_Device.GetDevice(), &l_Info, m_Context.GetAllocator(), &m_GBufferSampler), "VulkanRenderer: GeometryBuffer vkCreateSampler failed");
		}

		{
			VkSamplerCreateInfo l_Info{};
			l_Info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			l_Info.magFilter = VK_FILTER_LINEAR;
			l_Info.minFilter = VK_FILTER_LINEAR;
			l_Info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			l_Info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			l_Info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			l_Info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			l_Info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			l_Info.compareEnable = VK_TRUE;
			l_Info.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			l_Info.maxLod = 0.0f;
			l_Info.maxAnisotropy = 1.0f;
			Utilities::VulkanUtilities::VKCheck(vkCreateSampler(m_Device.GetDevice(), &l_Info, m_Context.GetAllocator(), &m_ShadowMapSampler), "VulkanRenderer: shadow vkCreateSampler failed");
		}

		{
			VkImageCreateInfo l_Info{};
			l_Info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			l_Info.imageType = VK_IMAGE_TYPE_2D;
			l_Info.extent = { s_ShadowMapSize, s_ShadowMapSize, 1 };
			l_Info.mipLevels = 1;
			l_Info.arrayLayers = 1;
			l_Info.format = m_SceneViewportDepthFormat;
			l_Info.tiling = VK_IMAGE_TILING_OPTIMAL;
			l_Info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			l_Info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			l_Info.samples = VK_SAMPLE_COUNT_1_BIT;
			l_Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			m_Allocator.CreateImage(l_Info, m_ShadowMapImage, m_ShadowMapAllocation);

			VkImageViewCreateInfo l_ViewInfo{};
			l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			l_ViewInfo.image = m_ShadowMapImage;
			l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			l_ViewInfo.format = m_SceneViewportDepthFormat;
			l_ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			l_ViewInfo.subresourceRange.levelCount = 1;
			l_ViewInfo.subresourceRange.layerCount = 1;
			Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device.GetDevice(), &l_ViewInfo, m_Context.GetAllocator(), &m_ShadowMapView), "VulkanRenderer: shadow vkCreateImageView failed");

			VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(0);
			m_Command.Begin(0);

			VkImageMemoryBarrier2 l_Barrier{};
			l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
			l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			l_Barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			l_Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			l_Barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			l_Barrier.image = m_ShadowMapImage;
			l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

			VkDependencyInfo l_DependencyInfo{};
			l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			l_DependencyInfo.imageMemoryBarrierCount = 1;
			l_DependencyInfo.pImageMemoryBarriers = &l_Barrier;
			vkCmdPipelineBarrier2(l_CommandBuffer, &l_DependencyInfo);

			m_Command.End(0);

			VkSubmitInfo l_Submit{};
			l_Submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			l_Submit.commandBufferCount = 1;
			l_Submit.pCommandBuffers = &l_CommandBuffer;
			vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &l_Submit, VK_NULL_HANDLE);
			vkQueueWaitIdle(m_Device.GetGraphicsQueue());
		}

		m_GBuffer.Initialize(m_Context, m_Device, m_Allocator, m_SceneViewportWidth, m_SceneViewportHeight);

		m_LightingUBOs.reserve(m_FramesInFlight);
		for (uint32_t i = 0; i < m_FramesInFlight; ++i)
		{
			m_LightingUBOs.emplace_back(m_Allocator, sizeof(LightingUniformData));
		}

		{
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
				Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Device.GetDevice(), &l_PoolInfo, m_Context.GetAllocator(), &m_LightingDescriptorPools[i]), "VulkanRenderer: vkCreateDescriptorPool failed");
			}
		}

		CreateShadowPipeline();
		CreateGeometryBufferPipeline();
		CreateLightingPipeline();

		TR_CORE_TRACE("VulkanRenderer: deferred resources initialized");
	}

	void VulkanRenderer::ShutdownDeferredResources()
	{
		const VkDevice l_Device = m_Device.GetDevice();
		if (l_Device == VK_NULL_HANDLE)
		{
			return;
		}

		m_LightingUBOs.clear();
		DestroyDeferredPipelines();

		for (VkDescriptorPool& it_Pool : m_LightingDescriptorPools)
		{
			if (it_Pool != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorPool(l_Device, it_Pool, m_Context.GetAllocator());
				it_Pool = VK_NULL_HANDLE;
			}
		}
		m_LightingDescriptorPools.clear();

		m_GBuffer.Shutdown();

		if (m_ShadowMapView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(l_Device, m_ShadowMapView, m_Context.GetAllocator());
			m_ShadowMapView = VK_NULL_HANDLE;
		}

		if (m_ShadowMapImage != VK_NULL_HANDLE)
		{
			m_Allocator.DestroyImage(m_ShadowMapImage, m_ShadowMapAllocation);
			m_ShadowMapImage = VK_NULL_HANDLE;
			m_ShadowMapAllocation = VK_NULL_HANDLE;
		}

		if (m_ShadowMapSampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(l_Device, m_ShadowMapSampler, m_Context.GetAllocator());
			m_ShadowMapSampler = VK_NULL_HANDLE;
		}

		if (m_GBufferSampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(l_Device, m_GBufferSampler, m_Context.GetAllocator());
			m_GBufferSampler = VK_NULL_HANDLE;
		}

		m_WhiteTexture.Destroy();
		m_DescriptorAllocator.Shutdown();
	}

	void VulkanRenderer::RecreateDeferredResources()
	{
		if (m_SceneViewportWidth == 0 || m_SceneViewportHeight == 0)
		{
			return;
		}

		if (m_GBuffer.IsValid())
		{
			m_ResourceStateTracker.ForgetImage(m_GBuffer.GetAlbedoImage());
			m_ResourceStateTracker.ForgetImage(m_GBuffer.GetNormalImage());
			m_ResourceStateTracker.ForgetImage(m_GBuffer.GetMaterialImage());
		}

		m_GBuffer.Recreate(m_SceneViewportWidth, m_SceneViewportHeight);
	}

	void VulkanRenderer::CreateShadowPipeline()
	{
		m_ShaderLibrary.Load("Shadow.vert", "Assets/Shaders/Shadow.vert.spv", ShaderStage::Vertex);
		m_ShaderLibrary.Load("Shadow.frag", "Assets/Shaders/Shadow.frag.spv", ShaderStage::Fragment);

		const std::vector<uint32_t>& l_VertexShaderSPV = *m_ShaderLibrary.GetSpirV("Shadow.vert");
		const std::vector<uint32_t>& l_FragmentShaderSPV = *m_ShaderLibrary.GetSpirV("Shadow.frag");

		VkDescriptorSetLayoutCreateInfo l_EmptySetLayout{};
		l_EmptySetLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device.GetDevice(), &l_EmptySetLayout, m_Context.GetAllocator(), &m_ShadowTextureSetLayout), "Shadow: vkCreateDescriptorSetLayout failed");

		VkPushConstantRange l_PushConstantRange{};
		l_PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		l_PushConstantRange.offset = 0;
		l_PushConstantRange.size = sizeof(ShadowPushConstants);

		VkPipelineLayoutCreateInfo l_LayoutInfo{};
		l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		l_LayoutInfo.setLayoutCount = 1;
		l_LayoutInfo.pSetLayouts = &m_ShadowTextureSetLayout;
		l_LayoutInfo.pushConstantRangeCount = 1;
		l_LayoutInfo.pPushConstantRanges = &l_PushConstantRange;
		Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device.GetDevice(), &l_LayoutInfo, m_Context.GetAllocator(), &m_ShadowPipelineLayout), "Shadow: vkCreatePipelineLayout failed");

		auto l_CreateModule = [&](const std::vector<uint32_t>& spv) -> VkShaderModule
			{
				VkShaderModuleCreateInfo l_Info{};
				l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				l_Info.codeSize = spv.size() * 4;
				l_Info.pCode = spv.data();
				VkShaderModule l_ShaderModule = VK_NULL_HANDLE;
				Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device.GetDevice(), &l_Info, m_Context.GetAllocator(), &l_ShaderModule), "vkCreateShaderModule");

				return l_ShaderModule;
			};

		const VkShaderModule l_VertexShaderModule = l_CreateModule(l_VertexShaderSPV);
		const VkShaderModule l_FragmentShaderModule = l_CreateModule(l_FragmentShaderSPV);

		VkPipelineShaderStageCreateInfo l_Stages[2]{};
		l_Stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_Stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		l_Stages[0].module = l_VertexShaderModule;
		l_Stages[0].pName = "main";
		l_Stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_Stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		l_Stages[1].module = l_FragmentShaderModule;
		l_Stages[1].pName = "main";

		VkVertexInputBindingDescription l_Binding{};
		l_Binding.binding = 0;
		l_Binding.stride = sizeof(Geometry::Vertex);
		l_Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription l_VertexInputAttributeDescriptions[3]{};
		l_VertexInputAttributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex, Position) };
		l_VertexInputAttributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex, Normal) };
		l_VertexInputAttributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Geometry::Vertex, UV) };

		VkPipelineVertexInputStateCreateInfo l_PipelineVertexInputStateCreateInfo{};
		l_PipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		l_PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
		l_PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &l_Binding;
		l_PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 3;
		l_PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = l_VertexInputAttributeDescriptions;

		VkPipelineInputAssemblyStateCreateInfo l_PipelineInputAssemblyStateCreateInfo{};
		l_PipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		l_PipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo l_PipelineViewportStateCreateInfo{};
		l_PipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		l_PipelineViewportStateCreateInfo.viewportCount = 1;
		l_PipelineViewportStateCreateInfo.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo l_PipelineRasterizationStateCreateInfo{};
		l_PipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		l_PipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		l_PipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
		l_PipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		l_PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
		l_PipelineRasterizationStateCreateInfo.depthBiasEnable = VK_TRUE;
		l_PipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 2.0f;
		l_PipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 2.5f;

		VkPipelineMultisampleStateCreateInfo l_PipelineMultisampleStateCreateInfo{};
		l_PipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		l_PipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo l_PipelineDepthStencilStateCreateInfo{};
		l_PipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		l_PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
		l_PipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
		l_PipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		VkPipelineColorBlendStateCreateInfo l_PipelineColorBlendStateCreateInfo{};
		l_PipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		const VkDynamicState l_DynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo l_PipelineDynamicStateCreateInfo{};
		l_PipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		l_PipelineDynamicStateCreateInfo.dynamicStateCount = 2;
		l_PipelineDynamicStateCreateInfo.pDynamicStates = l_DynamicState;

		VkPipelineRenderingCreateInfo l_PipelineRenderingCreateInfo{};
		l_PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		l_PipelineRenderingCreateInfo.depthAttachmentFormat = m_SceneViewportDepthFormat;
		l_PipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		VkGraphicsPipelineCreateInfo l_GraphicsPipelineCreateInfo{};
		l_GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		l_GraphicsPipelineCreateInfo.pNext = &l_PipelineRenderingCreateInfo;
		l_GraphicsPipelineCreateInfo.stageCount = 2;
		l_GraphicsPipelineCreateInfo.pStages = l_Stages;
		l_GraphicsPipelineCreateInfo.pVertexInputState = &l_PipelineVertexInputStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pInputAssemblyState = &l_PipelineInputAssemblyStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pViewportState = &l_PipelineViewportStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pRasterizationState = &l_PipelineRasterizationStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pMultisampleState = &l_PipelineMultisampleStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pDepthStencilState = &l_PipelineDepthStencilStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pColorBlendState = &l_PipelineColorBlendStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pDynamicState = &l_PipelineDynamicStateCreateInfo;
		l_GraphicsPipelineCreateInfo.layout = m_ShadowPipelineLayout;

		Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &l_GraphicsPipelineCreateInfo, m_Context.GetAllocator(), &m_ShadowPipeline), "Shadow: vkCreateGraphicsPipelines failed");

		vkDestroyShaderModule(m_Device.GetDevice(), l_VertexShaderModule, m_Context.GetAllocator());
		vkDestroyShaderModule(m_Device.GetDevice(), l_FragmentShaderModule, m_Context.GetAllocator());
	}

	void VulkanRenderer::CreateGeometryBufferPipeline()
	{
		m_ShaderLibrary.Load("GeometryBuffer.vert", "Assets/Shaders/GeometryBuffer.vert.spv", ShaderStage::Vertex);
		m_ShaderLibrary.Load("GeometryBuffer.frag", "Assets/Shaders/GeometryBuffer.frag.spv", ShaderStage::Fragment);

		const std::vector<uint32_t>& l_VertexShaderSPV = *m_ShaderLibrary.GetSpirV("GeometryBuffer.vert");
		const std::vector<uint32_t>& l_FragmentShaderSPV = *m_ShaderLibrary.GetSpirV("GeometryBuffer.frag");

		VkDescriptorSetLayoutBinding l_TexBinding{};
		l_TexBinding.binding = 0;
		l_TexBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		l_TexBinding.descriptorCount = 1;
		l_TexBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo l_SetLayoutInfo{};
		l_SetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		l_SetLayoutInfo.bindingCount = 1;
		l_SetLayoutInfo.pBindings = &l_TexBinding;
		Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device.GetDevice(), &l_SetLayoutInfo, m_Context.GetAllocator(), &m_GBufferTextureSetLayout), "GeometryBuffer: vkCreateDescriptorSetLayout failed");

		VkPushConstantRange l_PushConstantRange{};
		l_PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		l_PushConstantRange.offset = 0;
		l_PushConstantRange.size = sizeof(GeometryBufferPushConstants);

		VkPipelineLayoutCreateInfo l_LayoutInfo{};
		l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		l_LayoutInfo.setLayoutCount = 1;
		l_LayoutInfo.pSetLayouts = &m_GBufferTextureSetLayout;
		l_LayoutInfo.pushConstantRangeCount = 1;
		l_LayoutInfo.pPushConstantRanges = &l_PushConstantRange;
		Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device.GetDevice(), &l_LayoutInfo, m_Context.GetAllocator(), &m_GBufferPipelineLayout), "GeometryBuffer: vkCreatePipelineLayout failed");

		auto l_CreateModule = [&](const std::vector<uint32_t>& spv) -> VkShaderModule
			{
				VkShaderModuleCreateInfo l_Info{};
				l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				l_Info.codeSize = spv.size() * 4;
				l_Info.pCode = spv.data();
				VkShaderModule l_ShaderModule = VK_NULL_HANDLE;
				Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device.GetDevice(), &l_Info, m_Context.GetAllocator(), &l_ShaderModule), "vkCreateShaderModule");

				return l_ShaderModule;
			};

		const VkShaderModule l_VertexShaderModule = l_CreateModule(l_VertexShaderSPV);
		const VkShaderModule l_FragmentShaderModule = l_CreateModule(l_FragmentShaderSPV);

		VkPipelineShaderStageCreateInfo l_Stages[2]{};
		l_Stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT,   l_VertexShaderModule, "main" };
		l_Stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, l_FragmentShaderModule, "main" };

		VkVertexInputBindingDescription l_Binding{};
		l_Binding.binding = 0;
		l_Binding.stride = sizeof(Geometry::Vertex);
		l_Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription l_VertexInputAttributeDescriptions[3]{};
		l_VertexInputAttributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex, Position) };
		l_VertexInputAttributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex, Normal) };
		l_VertexInputAttributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Geometry::Vertex, UV) };

		VkPipelineVertexInputStateCreateInfo l_PipelineVertexInputStateCreateInfo{};
		l_PipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		l_PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
		l_PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &l_Binding;
		l_PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 3;
		l_PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = l_VertexInputAttributeDescriptions;

		VkPipelineInputAssemblyStateCreateInfo l_PipelineInputAssemblyStateCreateInfo{};
		l_PipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		l_PipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo l_PipelineViewportStateCreateInfo{};
		l_PipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		l_PipelineViewportStateCreateInfo.viewportCount = l_PipelineViewportStateCreateInfo.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo l_PipelineRasterizationStateCreateInfo{};
		l_PipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		l_PipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		l_PipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
		l_PipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		l_PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo l_PipelineMultisampleStateCreateInfo{};
		l_PipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		l_PipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo l_PipelineDepthStencilStateCreateInfo{};
		l_PipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		l_PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
		l_PipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
		l_PipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		VkPipelineColorBlendAttachmentState l_BlendAtt{};
		l_BlendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		const VkPipelineColorBlendAttachmentState l_BlendAtts[3] = { l_BlendAtt, l_BlendAtt, l_BlendAtt };
		VkPipelineColorBlendStateCreateInfo l_PipelineColorBlendStateCreateInfo{};
		l_PipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		l_PipelineColorBlendStateCreateInfo.attachmentCount = 3;
		l_PipelineColorBlendStateCreateInfo.pAttachments = l_BlendAtts;

		const VkDynamicState l_DynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo l_PipelineDynamicStateCreateInfo{};
		l_PipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		l_PipelineDynamicStateCreateInfo.dynamicStateCount = 2;
		l_PipelineDynamicStateCreateInfo.pDynamicStates = l_DynamicState;

		const VkFormat l_ColorFmts[3] = { TextureFormatToVkFormat(VulkanGeometryBuffer::AlbedoFormat), TextureFormatToVkFormat(VulkanGeometryBuffer::NormalFormat), TextureFormatToVkFormat(VulkanGeometryBuffer::MaterialFormat) };
		VkPipelineRenderingCreateInfo l_PipelineRenderingCreateInfo{};
		l_PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		l_PipelineRenderingCreateInfo.colorAttachmentCount = 3;
		l_PipelineRenderingCreateInfo.pColorAttachmentFormats = l_ColorFmts;
		l_PipelineRenderingCreateInfo.depthAttachmentFormat = m_SceneViewportDepthFormat;

		VkGraphicsPipelineCreateInfo l_Info{};
		l_Info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		l_Info.pNext = &l_PipelineRenderingCreateInfo;
		l_Info.stageCount = 2;
		l_Info.pStages = l_Stages;
		l_Info.pVertexInputState = &l_PipelineVertexInputStateCreateInfo;
		l_Info.pInputAssemblyState = &l_PipelineInputAssemblyStateCreateInfo;
		l_Info.pViewportState = &l_PipelineViewportStateCreateInfo;
		l_Info.pRasterizationState = &l_PipelineRasterizationStateCreateInfo;
		l_Info.pMultisampleState = &l_PipelineMultisampleStateCreateInfo;
		l_Info.pDepthStencilState = &l_PipelineDepthStencilStateCreateInfo;
		l_Info.pColorBlendState = &l_PipelineColorBlendStateCreateInfo;
		l_Info.pDynamicState = &l_PipelineDynamicStateCreateInfo;
		l_Info.layout = m_GBufferPipelineLayout;

		Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &l_Info, m_Context.GetAllocator(), &m_GBufferPipeline), "GeometryBuffer: vkCreateGraphicsPipelines failed");

		vkDestroyShaderModule(m_Device.GetDevice(), l_VertexShaderModule, m_Context.GetAllocator());
		vkDestroyShaderModule(m_Device.GetDevice(), l_FragmentShaderModule, m_Context.GetAllocator());
	}

	void VulkanRenderer::CreateLightingPipeline()
	{
		m_ShaderLibrary.Load("Lighting.vert", "Assets/Shaders/Lighting.vert.spv", ShaderStage::Vertex);
		m_ShaderLibrary.Load("Lighting.frag", "Assets/Shaders/Lighting.frag.spv", ShaderStage::Fragment);

		const std::vector<uint32_t>& l_VertexShaderSPV = *m_ShaderLibrary.GetSpirV("Lighting.vert");
		const std::vector<uint32_t>& l_FragmentShaderSPV = *m_ShaderLibrary.GetSpirV("Lighting.frag");

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
		Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device.GetDevice(), &l_GBufSetInfo, m_Context.GetAllocator(), &m_LightingGBufferSetLayout), "Lighting: vkCreateDescriptorSetLayout (GeometryBuffer set) failed");

		VkDescriptorSetLayoutBinding l_UBOBinding{};
		l_UBOBinding.binding = 0;
		l_UBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		l_UBOBinding.descriptorCount = 1;
		l_UBOBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo l_UBOSetInfo{};
		l_UBOSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		l_UBOSetInfo.bindingCount = 1;
		l_UBOSetInfo.pBindings = &l_UBOBinding;
		Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device.GetDevice(), &l_UBOSetInfo, m_Context.GetAllocator(), &m_LightingUBOSetLayout), "Lighting: vkCreateDescriptorSetLayout (UBO set) failed");

		VkPushConstantRange l_PushConstantRange{};
		l_PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		l_PushConstantRange.offset = 0;
		l_PushConstantRange.size = sizeof(LightingPushConstants);

		const VkDescriptorSetLayout l_Layouts[2] = { m_LightingGBufferSetLayout, m_LightingUBOSetLayout };
		VkPipelineLayoutCreateInfo l_LayoutInfo{};
		l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		l_LayoutInfo.setLayoutCount = 2;
		l_LayoutInfo.pSetLayouts = l_Layouts;
		l_LayoutInfo.pushConstantRangeCount = 1;
		l_LayoutInfo.pPushConstantRanges = &l_PushConstantRange;
		Utilities::VulkanUtilities::VKCheck(vkCreatePipelineLayout(m_Device.GetDevice(), &l_LayoutInfo, m_Context.GetAllocator(), &m_LightingPipelineLayout), "Lighting: vkCreatePipelineLayout failed");

		auto l_CreateModule = [&](const std::vector<uint32_t>& spv) -> VkShaderModule
			{
				VkShaderModuleCreateInfo l_Info{};
				l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				l_Info.codeSize = spv.size() * 4;
				l_Info.pCode = spv.data();
				VkShaderModule l_ShaderModule = VK_NULL_HANDLE;
				Utilities::VulkanUtilities::VKCheck(vkCreateShaderModule(m_Device.GetDevice(), &l_Info, m_Context.GetAllocator(), &l_ShaderModule), "vkCreateShaderModule");

				return l_ShaderModule;
			};

		const VkShaderModule l_VertexShaderModule = l_CreateModule(l_VertexShaderSPV);
		const VkShaderModule l_FragmentShaderModule = l_CreateModule(l_FragmentShaderSPV);

		VkPipelineShaderStageCreateInfo l_Stages[2]{};
		l_Stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT,   l_VertexShaderModule, "main" };
		l_Stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, l_FragmentShaderModule, "main" };

		VkPipelineVertexInputStateCreateInfo l_PipelineVertexInputStateCreateInfo{};
		l_PipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkPipelineInputAssemblyStateCreateInfo l_PipelineInputAssemblyStateCreateInfo{};
		l_PipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		l_PipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo l_PipelineViewportStateCreateInfo{};
		l_PipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		l_PipelineViewportStateCreateInfo.viewportCount = l_PipelineViewportStateCreateInfo.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo l_PipelineRasterizationStateCreateInfo{};
		l_PipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		l_PipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		l_PipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
		l_PipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		l_PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo l_PipelineMultisampleStateCreateInfo{};
		l_PipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		l_PipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo l_PipelineDepthStencilStateCreateInfo{};
		l_PipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		VkPipelineColorBlendAttachmentState l_BlendAtt{};
		l_BlendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo l_PipelineColorBlendStateCreateInfo{};
		l_PipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		l_PipelineColorBlendStateCreateInfo.attachmentCount = 1;
		l_PipelineColorBlendStateCreateInfo.pAttachments = &l_BlendAtt;

		const VkDynamicState l_DynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo l_PipelineDynamicStateCreateInfo{};
		l_PipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		l_PipelineDynamicStateCreateInfo.dynamicStateCount = 2;
		l_PipelineDynamicStateCreateInfo.pDynamicStates = l_DynamicState;

		const VkFormat l_ColorFormat = m_Swapchain.GetImageFormat();
		VkPipelineRenderingCreateInfo l_PipelineRenderingCreateInfo{};
		l_PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		l_PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		l_PipelineRenderingCreateInfo.pColorAttachmentFormats = &l_ColorFormat;

		VkGraphicsPipelineCreateInfo l_GraphicsPipelineCreateInfo{};
		l_GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		l_GraphicsPipelineCreateInfo.pNext = &l_PipelineRenderingCreateInfo;
		l_GraphicsPipelineCreateInfo.stageCount = 2;
		l_GraphicsPipelineCreateInfo.pStages = l_Stages;
		l_GraphicsPipelineCreateInfo.pVertexInputState = &l_PipelineVertexInputStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pInputAssemblyState = &l_PipelineInputAssemblyStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pViewportState = &l_PipelineViewportStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pRasterizationState = &l_PipelineRasterizationStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pMultisampleState = &l_PipelineMultisampleStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pDepthStencilState = &l_PipelineDepthStencilStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pColorBlendState = &l_PipelineColorBlendStateCreateInfo;
		l_GraphicsPipelineCreateInfo.pDynamicState = &l_PipelineDynamicStateCreateInfo;
		l_GraphicsPipelineCreateInfo.layout = m_LightingPipelineLayout;

		Utilities::VulkanUtilities::VKCheck(vkCreateGraphicsPipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &l_GraphicsPipelineCreateInfo, m_Context.GetAllocator(), &m_LightingPipeline), "Lighting: vkCreateGraphicsPipelines failed");

		vkDestroyShaderModule(m_Device.GetDevice(), l_VertexShaderModule, m_Context.GetAllocator());
		vkDestroyShaderModule(m_Device.GetDevice(), l_FragmentShaderModule, m_Context.GetAllocator());
	}

	void VulkanRenderer::DestroyDeferredPipelines()
	{
		const VkDevice l_Device = m_Device.GetDevice();
		const VkAllocationCallbacks* l_AllocationCallback = m_Context.GetAllocator();

		auto l_Destroy = [&](VkPipeline& pipe, VkPipelineLayout& layout, VkDescriptorSetLayout& set0, VkDescriptorSetLayout* set1 = nullptr)
			{
				if (pipe != VK_NULL_HANDLE)
				{
					vkDestroyPipeline(l_Device, pipe, l_AllocationCallback);
					pipe = VK_NULL_HANDLE;
				}
				
				if (layout != VK_NULL_HANDLE)
				{
					vkDestroyPipelineLayout(l_Device, layout, l_AllocationCallback);
					layout = VK_NULL_HANDLE;
				}

				if (set0 != VK_NULL_HANDLE)
				{
					vkDestroyDescriptorSetLayout(l_Device, set0, l_AllocationCallback);
					set0 = VK_NULL_HANDLE;
				}

				if (set1 && *set1 != VK_NULL_HANDLE)
				{
					vkDestroyDescriptorSetLayout(l_Device, *set1, l_AllocationCallback);
					*set1 = VK_NULL_HANDLE;
				}
			};

		l_Destroy(m_ShadowPipeline, m_ShadowPipelineLayout, m_ShadowTextureSetLayout);
		l_Destroy(m_GBufferPipeline, m_GBufferPipelineLayout, m_GBufferTextureSetLayout);
		l_Destroy(m_LightingPipeline, m_LightingPipelineLayout, m_LightingGBufferSetLayout, &m_LightingUBOSetLayout);
	}

	VkDescriptorSet VulkanRenderer::BuildTextureDescriptorSet(VkImageView imageView, VkSampler sampler)
	{
		const VkDescriptorSet l_DescriptorSet = m_DescriptorAllocator.Allocate(m_CurrentFrameIndex, m_GBufferTextureSetLayout);

		VkDescriptorImageInfo l_ImageInfo{};
		l_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		l_ImageInfo.imageView = imageView;
		l_ImageInfo.sampler = sampler;

		VkWriteDescriptorSet l_Write{};
		l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		l_Write.dstSet = l_DescriptorSet;
		l_Write.dstBinding = 0;
		l_Write.descriptorCount = 1;
		l_Write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		l_Write.pImageInfo = &l_ImageInfo;

		vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &l_Write, 0, nullptr);

		return l_DescriptorSet;
	}

	VkDescriptorSet VulkanRenderer::BuildGeometryBufferDescriptorSet()
	{
		VkDescriptorSetAllocateInfo l_AllocInfo{};
		l_AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		l_AllocInfo.descriptorPool = m_LightingDescriptorPools[m_CurrentFrameIndex];
		l_AllocInfo.descriptorSetCount = 1;
		l_AllocInfo.pSetLayouts = &m_LightingGBufferSetLayout;

		VkDescriptorSet l_DescriptorSet = VK_NULL_HANDLE;
		Utilities::VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Device.GetDevice(), &l_AllocInfo, &l_DescriptorSet), "VulkanRenderer: vkAllocateDescriptorSets (GeometryBuffer set) failed");

		const VkImageLayout l_SRO = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		const VkImageLayout l_DSR = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo l_DescriptorImageInfos[5]{};
		l_DescriptorImageInfos[0] = { m_GBufferSampler, m_GBuffer.GetAlbedoView(), l_SRO };
		l_DescriptorImageInfos[1] = { m_GBufferSampler, m_GBuffer.GetNormalView(), l_SRO };
		l_DescriptorImageInfos[2] = { m_GBufferSampler, m_GBuffer.GetMaterialView(), l_SRO };
		l_DescriptorImageInfos[3] = { m_GBufferSampler, m_SceneViewportDepthImageView, l_DSR };
		l_DescriptorImageInfos[4] = { m_ShadowMapSampler, m_ShadowMapView, l_DSR };

		VkWriteDescriptorSet l_Writes[5]{};
		for (uint32_t i = 0; i < 5; ++i)
		{
			l_Writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			l_Writes[i].dstSet = l_DescriptorSet;
			l_Writes[i].dstBinding = i;
			l_Writes[i].descriptorCount = 1;
			l_Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			l_Writes[i].pImageInfo = &l_DescriptorImageInfos[i];
		}

		vkUpdateDescriptorSets(m_Device.GetDevice(), 5, l_Writes, 0, nullptr);

		return l_DescriptorSet;
	}

	void VulkanRenderer::BeginShadowPass(const glm::mat4& lightSpaceMatrix)
	{
		if (!m_FrameBegun)
		{
			return;
		}

		if (m_ScenePassRecording)
		{
			const VkCommandBuffer l_EndCB = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
			vkCmdEndRendering(l_EndCB);
			m_ScenePassRecording = false;
		}

		m_CurrentLightSpaceMatrix = lightSpaceMatrix;
		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

		{
			VkImageMemoryBarrier2 l_Barrier{};
			l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			l_Barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			l_Barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			l_Barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			l_Barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			l_Barrier.image = m_ShadowMapImage;
			l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

			VkDependencyInfo l_DependencyInfo{};
			l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			l_DependencyInfo.imageMemoryBarrierCount = 1;
			l_DependencyInfo.pImageMemoryBarriers = &l_Barrier;
			vkCmdPipelineBarrier2(l_CommandBuffer, &l_DependencyInfo);
		}

		VkRenderingAttachmentInfo l_DepthRenderingAttachmentInfo{};
		l_DepthRenderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		l_DepthRenderingAttachmentInfo.imageView = m_ShadowMapView;
		l_DepthRenderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		l_DepthRenderingAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_DepthRenderingAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_DepthRenderingAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };

		VkRenderingInfo l_RenderingInfo{};
		l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		l_RenderingInfo.renderArea = { { 0, 0 }, { s_ShadowMapSize, s_ShadowMapSize } };
		l_RenderingInfo.layerCount = 1;
		l_RenderingInfo.pDepthAttachment = &l_DepthRenderingAttachmentInfo;

		vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);

		VkViewport l_Viewport{ 0.0f, 0.0f, static_cast<float>(s_ShadowMapSize), static_cast<float>(s_ShadowMapSize), 0.0f, 1.0f };
		VkRect2D  l_Scissor{ { 0, 0 }, { s_ShadowMapSize, s_ShadowMapSize } };
		vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);
		vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);
		vkCmdBindPipeline(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowPipeline);

		m_ShadowPassRecording = true;
	}

	void VulkanRenderer::EndShadowPass()
	{
		if (!m_ShadowPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		vkCmdEndRendering(l_CommandBuffer);

		{
			VkImageMemoryBarrier2 l_Barrier{};
			l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			l_Barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			l_Barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			l_Barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			l_Barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			l_Barrier.image = m_ShadowMapImage;
			l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

			VkDependencyInfo l_DependencyInfo{};
			l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			l_DependencyInfo.imageMemoryBarrierCount = 1;
			l_DependencyInfo.pImageMemoryBarriers = &l_Barrier;
			vkCmdPipelineBarrier2(l_CommandBuffer, &l_DependencyInfo);
		}

		m_ShadowPassRecording = false;
	}

	void VulkanRenderer::DrawMeshShadow(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& lightSpaceMVP)
	{
		if (!m_ShadowPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

		VkBuffer l_VertexBuffer = reinterpret_cast<VkBuffer>(vertexBuffer.GetNativeHandle());
		VkBuffer l_IndexBuffer = reinterpret_cast<VkBuffer>(indexBuffer.GetNativeHandle());
		VkDeviceSize l_Off = 0;

		vkCmdBindVertexBuffers(l_CommandBuffer, 0, 1, &l_VertexBuffer, &l_Off);
		vkCmdBindIndexBuffer(l_CommandBuffer, l_IndexBuffer, 0, ToVkIndexType(indexBuffer.GetIndexType()));

		ShadowPushConstants l_PushConstantRange{};
		l_PushConstantRange.LightSpaceModelViewProjection = lightSpaceMVP;
		vkCmdPushConstants(l_CommandBuffer, m_ShadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowPushConstants), &l_PushConstantRange);

		vkCmdDrawIndexed(l_CommandBuffer, indexCount, 1, 0, 0, 0);
	}

	void VulkanRenderer::BeginGeometryPass()
	{
		if (!m_FrameBegun || !m_GBuffer.IsValid())
		{
			return;
		}

		if (m_ScenePassRecording)
		{
			const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
			vkCmdEndRendering(l_CommandBuffer);
			m_ScenePassRecording = false;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		const VkImageSubresourceRange l_ColorRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		const VulkanImageTransitionState l_CAW = BuildTransitionState(ImageTransitionPreset::ColorAttachmentWrite);

		TransitionImageResource(l_CommandBuffer, m_GBuffer.GetAlbedoImage(), l_ColorRange, l_CAW);
		TransitionImageResource(l_CommandBuffer, m_GBuffer.GetNormalImage(), l_ColorRange, l_CAW);
		TransitionImageResource(l_CommandBuffer, m_GBuffer.GetMaterialImage(), l_ColorRange, l_CAW);
		TransitionImageResource(l_CommandBuffer, m_SceneViewportDepthImage, BuildDepthSubresourceRange(), BuildTransitionState(ImageTransitionPreset::DepthAttachmentWrite));

		VkClearValue l_Clear{};
		VkRenderingAttachmentInfo l_ColorRenderingAttachmentInfos[3]{};
		const VkImageView l_Views[3] = { m_GBuffer.GetAlbedoView(), m_GBuffer.GetNormalView(), m_GBuffer.GetMaterialView() };
		for (uint32_t i = 0; i < 3; ++i)
		{
			l_ColorRenderingAttachmentInfos[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			l_ColorRenderingAttachmentInfos[i].imageView = l_Views[i];
			l_ColorRenderingAttachmentInfos[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			l_ColorRenderingAttachmentInfos[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			l_ColorRenderingAttachmentInfos[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			l_ColorRenderingAttachmentInfos[i].clearValue = l_Clear;
		}

		VkClearValue l_DepthClear{};
		l_DepthClear.depthStencil = { 1.0f, 0 };
		VkRenderingAttachmentInfo l_DepthRenderingAttachmentInfo{};
		l_DepthRenderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		l_DepthRenderingAttachmentInfo.imageView = m_SceneViewportDepthImageView;
		l_DepthRenderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		l_DepthRenderingAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_DepthRenderingAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_DepthRenderingAttachmentInfo.clearValue = l_DepthClear;

		VkRenderingInfo l_RenderingInfo{};
		l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		l_RenderingInfo.renderArea = { { 0, 0 }, { m_SceneViewportWidth, m_SceneViewportHeight } };
		l_RenderingInfo.layerCount = 1;
		l_RenderingInfo.colorAttachmentCount = 3;
		l_RenderingInfo.pColorAttachments = l_ColorRenderingAttachmentInfos;
		l_RenderingInfo.pDepthAttachment = &l_DepthRenderingAttachmentInfo;

		vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);

		VkViewport l_Viewport{ 0.0f, 0.0f, static_cast<float>(m_SceneViewportWidth), static_cast<float>(m_SceneViewportHeight), 0.0f, 1.0f };
		VkRect2D l_Scissor{ { 0, 0 }, { m_SceneViewportWidth, m_SceneViewportHeight } };
		vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);
		vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);
		vkCmdBindPipeline(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GBufferPipeline);

		m_GeometryPassRecording = true;
	}

	void VulkanRenderer::EndGeometryPass()
	{
		if (!m_GeometryPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		vkCmdEndRendering(l_CommandBuffer);

		const VkImageSubresourceRange l_ColorRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		const VulkanImageTransitionState l_SRO = BuildTransitionState(ImageTransitionPreset::ShaderReadOnly);
		TransitionImageResource(l_CommandBuffer, m_GBuffer.GetAlbedoImage(), l_ColorRange, l_SRO);
		TransitionImageResource(l_CommandBuffer, m_GBuffer.GetNormalImage(), l_ColorRange, l_SRO);
		TransitionImageResource(l_CommandBuffer, m_GBuffer.GetMaterialImage(), l_ColorRange, l_SRO);
		TransitionImageResource(l_CommandBuffer, m_SceneViewportDepthImage, BuildDepthSubresourceRange(), BuildTransitionState(ImageTransitionPreset::DepthShaderReadOnly));

		m_GeometryPassRecording = false;
	}

	void VulkanRenderer::DrawMeshDeferred(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::mat4& viewProjection, const glm::vec4& color, Texture2D* albedoTexture)
	{
		if (!m_GeometryPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

		VkImageView l_View = m_WhiteTexture.GetImageView();
		VkSampler l_Sampler = m_WhiteTexture.GetSampler();

		if (albedoTexture)
		{
			if (const auto* l_VTex = dynamic_cast<VulkanTexture2D*>(albedoTexture))
			{
				l_View = l_VTex->GetImageView();
				l_Sampler = l_VTex->GetSampler();
			}
		}

		const VkDescriptorSet l_TexSet = BuildTextureDescriptorSet(l_View, l_Sampler);
		vkCmdBindDescriptorSets(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GBufferPipelineLayout, 0, 1, &l_TexSet, 0, nullptr);

		VkBuffer l_VertexBuffer = reinterpret_cast<VkBuffer>(vertexBuffer.GetNativeHandle());
		VkBuffer l_IndexBuffer = reinterpret_cast<VkBuffer>(indexBuffer.GetNativeHandle());
		VkDeviceSize l_Off = 0;
		vkCmdBindVertexBuffers(l_CommandBuffer, 0, 1, &l_VertexBuffer, &l_Off);
		vkCmdBindIndexBuffer(l_CommandBuffer, l_IndexBuffer, 0, ToVkIndexType(indexBuffer.GetIndexType()));

		GeometryBufferPushConstants l_PushConstantRange{};
		l_PushConstantRange.ModelViewProjection = viewProjection * model;
		l_PushConstantRange.Model = model;
		l_PushConstantRange.Color = color;
		vkCmdPushConstants(l_CommandBuffer, m_GBufferPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GeometryBufferPushConstants), &l_PushConstantRange);

		vkCmdDrawIndexed(l_CommandBuffer, indexCount, 1, 0, 0, 0);
	}

	void VulkanRenderer::BeginLightingPass()
	{
		if (!m_FrameBegun || m_SceneViewportImage == VK_NULL_HANDLE)
		{
			return;
		}

		if (m_GeometryPassRecording)
		{
			EndGeometryPass();
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		TransitionImageResource(l_CommandBuffer, m_SceneViewportImage, BuildColorSubresourceRange(), BuildTransitionState(ImageTransitionPreset::ColorAttachmentWrite));

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
		l_RenderingInfo.renderArea = { { 0, 0 }, { m_SceneViewportWidth, m_SceneViewportHeight } };
		l_RenderingInfo.layerCount = 1;
		l_RenderingInfo.colorAttachmentCount = 1;
		l_RenderingInfo.pColorAttachments = &l_ColorAtt;

		vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);

		VkViewport l_Viewport{ 0.0f, 0.0f, static_cast<float>(m_SceneViewportWidth), static_cast<float>(m_SceneViewportHeight), 0.0f, 1.0f };
		VkRect2D l_Scissor{ { 0, 0 }, { m_SceneViewportWidth, m_SceneViewportHeight } };
		vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);
		vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);
		vkCmdBindPipeline(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipeline);

		m_LightingPassRecording = true;
	}

	void VulkanRenderer::EndLightingPass()
	{
		if (!m_LightingPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		vkCmdEndRendering(l_CommandBuffer);
		TransitionImageResource(l_CommandBuffer, m_SceneViewportImage, BuildColorSubresourceRange(), BuildTransitionState(ImageTransitionPreset::ShaderReadOnly));
		m_LightingPassRecording = false;
	}

	void VulkanRenderer::UploadLights(const void* lightData, uint32_t byteSize)
	{
		if (m_CurrentFrameIndex >= static_cast<uint32_t>(m_LightingUBOs.size()))
		{
			return;
		}

		m_LightingUBOs[m_CurrentFrameIndex].SetData(lightData, static_cast<uint64_t>(byteSize));
	}

	void VulkanRenderer::DrawLightingQuad(const glm::mat4& invViewProjection, const glm::vec3& cameraPosition, float cameraNear, float cameraFar)
	{
		if (!m_LightingPassRecording)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		const VkDescriptorSet l_GBufSet = BuildGeometryBufferDescriptorSet();

		VkDescriptorSetAllocateInfo l_UBOAllocInfo{};
		l_UBOAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		l_UBOAllocInfo.descriptorPool = m_LightingDescriptorPools[m_CurrentFrameIndex];
		l_UBOAllocInfo.descriptorSetCount = 1;
		l_UBOAllocInfo.pSetLayouts = &m_LightingUBOSetLayout;

		VkDescriptorSet l_UBOSet = VK_NULL_HANDLE;
		Utilities::VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Device.GetDevice(), &l_UBOAllocInfo, &l_UBOSet), "VulkanRenderer: vkAllocateDescriptorSets (UBO set) failed");

		const VkDescriptorBufferInfo l_BufInfo = m_LightingUBOs[m_CurrentFrameIndex].GetDescriptorBufferInfo();

		VkWriteDescriptorSet l_UBOWrite{};
		l_UBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		l_UBOWrite.dstSet = l_UBOSet;
		l_UBOWrite.dstBinding = 0;
		l_UBOWrite.descriptorCount = 1;
		l_UBOWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		l_UBOWrite.pBufferInfo = &l_BufInfo;
		vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &l_UBOWrite, 0, nullptr);

		const VkDescriptorSet l_Sets[2] = { l_GBufSet, l_UBOSet };
		vkCmdBindDescriptorSets(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipelineLayout, 0, 2, l_Sets, 0, nullptr);

		const VulkanSwapchain::SceneColorPolicy& l_ColorPolicy = m_Swapchain.GetSceneColorPolicy();

		LightingPushConstants l_PushConstantRange{};
		l_PushConstantRange.InvViewProjection = invViewProjection;
		l_PushConstantRange.CameraPosition = glm::vec4(cameraPosition, 1.0f);
		l_PushConstantRange.ColorOutputTransfer = static_cast<uint32_t>(l_ColorPolicy.SceneOutputTransfer);
		l_PushConstantRange.CameraNear = cameraNear;
		l_PushConstantRange.CameraFar = cameraFar;
		vkCmdPushConstants(l_CommandBuffer, m_LightingPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightingPushConstants), &l_PushConstantRange);

		vkCmdDraw(l_CommandBuffer, 3, 1, 0, 0);
	}
}