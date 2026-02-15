#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"

#include "Trinity/Renderer/Vulkan/VulkanShaderInterop.h"
#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Platform/Window.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>

namespace Trinity
{
	namespace
	{
		uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties l_MemoryProperties{};
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &l_MemoryProperties);

			for (uint32_t it_Index = 0; it_Index < l_MemoryProperties.memoryTypeCount; ++it_Index)
			{
				if ((typeFilter & (1u << it_Index)) && (l_MemoryProperties.memoryTypes[it_Index].propertyFlags & properties) == properties)
				{
					return it_Index;
				}
			}

			TR_CORE_CRITICAL("Failed to find suitable Vulkan memory type for scene viewport image");
			std::abort();
		}
	}

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

	VulkanRenderer::VulkanRenderer(const Configuration& configuration) : Renderer(RendererAPI::VULKAN), m_Configuration(configuration)
	{

	}

	VulkanRenderer::~VulkanRenderer()
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
		m_Swapchain.Initialize(m_Context, m_Device, m_Window->GetWidth(), m_Window->GetHeight(), m_Configuration.m_ColorOutputPolicy);
		m_Sync.Initialize(m_Context, m_Device, m_FramesInFlight, m_Swapchain.GetImageCount());
		m_Command.Initialize(m_Context, m_Device, m_FramesInFlight);
		m_Pipeline.Initialize(m_Context, m_Device, m_Swapchain.GetImageFormat(), m_VertexShaderPath, m_FragmentShaderPath);
		m_ResourceStateTracker.Reset();
		m_ImGuiLayer = nullptr;
		m_ImGuiVulkanInitialized = false;
		m_SceneViewportHandle = nullptr;
	}

	void VulkanRenderer::Shutdown()
	{
		if (m_Device.GetDevice() != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_Device.GetDevice());
		}

		DestroySceneViewportTarget();

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

	void VulkanRenderer::SetSceneViewportSize(uint32_t width, uint32_t height)
	{
		if (m_SceneViewportWidth == width && m_SceneViewportHeight == height)
		{
			return;
		}

		m_SceneViewportWidth = width;
		m_SceneViewportHeight = height;

		if (m_Device.GetDevice() == VK_NULL_HANDLE)
		{
			return;
		}

		vkDeviceWaitIdle(m_Device.GetDevice());
		RecreateSceneViewportTarget();
	}

	void* VulkanRenderer::GetSceneViewportHandle() const
	{
		return m_SceneViewportHandle;
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

		Utilities::VulkanUtilities::VKCheck(vkCreateImage(m_Device.GetDevice(), &l_ImageCreateInfo, m_Context.GetAllocator(), &m_SceneViewportImage), "Failed vkCreateImage");

		VkMemoryRequirements l_MemoryRequirements{};
		vkGetImageMemoryRequirements(m_Device.GetDevice(), m_SceneViewportImage, &l_MemoryRequirements);

		VkMemoryAllocateInfo l_AllocateInfo{};
		l_AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		l_AllocateInfo.allocationSize = l_MemoryRequirements.size;
		l_AllocateInfo.memoryTypeIndex = FindMemoryType(m_Device.GetPhysicalDevice(), l_MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Utilities::VulkanUtilities::VKCheck(vkAllocateMemory(m_Device.GetDevice(), &l_AllocateInfo, m_Context.GetAllocator(), &m_SceneViewportImageMemory), "Failed vkAllocateMemory");
		Utilities::VulkanUtilities::VKCheck(vkBindImageMemory(m_Device.GetDevice(), m_SceneViewportImage, m_SceneViewportImageMemory, 0), "Failed vkBindImageMemory");

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

		if (m_SceneViewportImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_Device.GetDevice(), m_SceneViewportImageView, m_Context.GetAllocator());
			m_SceneViewportImageView = VK_NULL_HANDLE;
		}

		if (m_SceneViewportImage != VK_NULL_HANDLE)
		{
			vkDestroyImage(m_Device.GetDevice(), m_SceneViewportImage, m_Context.GetAllocator());
			m_ResourceStateTracker.ForgetImage(m_SceneViewportImage);
			m_SceneViewportImage = VK_NULL_HANDLE;
		}

		if (m_SceneViewportImageMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_Device.GetDevice(), m_SceneViewportImageMemory, m_Context.GetAllocator());
			m_SceneViewportImageMemory = VK_NULL_HANDLE;
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

		VkExtent2D l_TargetExtent = m_Swapchain.GetExtent();
		VkImageView l_TargetImageView = m_Swapchain.GetImageViews()[m_CurrentImageIndex];

		if (m_SceneViewportImage != VK_NULL_HANDLE)
		{
			TransitionImageResource(l_CommandBuffer, m_SceneViewportImage, l_ColorSubresourceRange, l_ColorAttachmentWriteState);
			l_TargetExtent = { m_SceneViewportWidth, m_SceneViewportHeight };
			l_TargetImageView = m_SceneViewportImageView;
		}
		else
		{
			TransitionImageResource(l_CommandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], l_ColorSubresourceRange, l_ColorAttachmentWriteState);
		}

		VkClearValue l_ClearColor{};
		l_ClearColor.color.float32[0] = 0.0008f;
		l_ClearColor.color.float32[1] = 0.0008f;
		l_ClearColor.color.float32[2] = 0.0008f;
		l_ClearColor.color.float32[3] = 1.0f;

		VkRenderingAttachmentInfo l_ColorAttachmentInfo{};
		l_ColorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		l_ColorAttachmentInfo.imageView = l_TargetImageView;
		l_ColorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		l_ColorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		l_ColorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		l_ColorAttachmentInfo.clearValue = l_ClearColor;

		VkRenderingInfo l_RenderingInfo{};
		l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		l_RenderingInfo.renderArea.offset = { 0, 0 };
		l_RenderingInfo.renderArea.extent = l_TargetExtent;
		l_RenderingInfo.layerCount = 1;
		l_RenderingInfo.colorAttachmentCount = 1;
		l_RenderingInfo.pColorAttachments = &l_ColorAttachmentInfo;

		vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);

		VkViewport l_Viewport{};
		l_Viewport.x = 0.0f;
		l_Viewport.y = 0.0f;
		l_Viewport.width = static_cast<float>(l_TargetExtent.width);
		l_Viewport.height = static_cast<float>(l_TargetExtent.height);
		l_Viewport.minDepth = 0.0f;
		l_Viewport.maxDepth = 1.0f;

		VkRect2D l_Scissor{};
		l_Scissor.offset = { 0, 0 };
		l_Scissor.extent = l_TargetExtent;

		vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);
		vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);

		m_Pipeline.Bind(l_CommandBuffer);
		ValidateSceneColorPolicy();

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

		if (m_SceneViewportImage != VK_NULL_HANDLE)
		{
			vkCmdEndRendering(l_CommandBuffer);
			TransitionImageResource(l_CommandBuffer, m_SceneViewportImage, l_ColorSubresourceRange, BuildTransitionState(ImageTransitionPreset::ShaderReadOnly));
			TransitionImageResource(l_CommandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], l_ColorSubresourceRange, l_ColorAttachmentWriteState);

			VkClearValue l_ClearColor{};
			l_ClearColor.color.float32[0] = 0.0008f;
			l_ClearColor.color.float32[1] = 0.0008f;
			l_ClearColor.color.float32[2] = 0.0008f;
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

		if (!m_FrameBegun)
		{
			return;
		}

		const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
		if (m_SceneViewportImage != VK_NULL_HANDLE)
		{
			const VkImageSubresourceRange l_ColorSubresourceRange = BuildColorSubresourceRange();
			const VulkanImageTransitionState l_ColorAttachmentWriteState = BuildTransitionState(ImageTransitionPreset::ColorAttachmentWrite);

			vkCmdEndRendering(l_CommandBuffer);
			TransitionImageResource(l_CommandBuffer, m_SceneViewportImage, l_ColorSubresourceRange, BuildTransitionState(ImageTransitionPreset::ShaderReadOnly));
			TransitionImageResource(l_CommandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], l_ColorSubresourceRange, l_ColorAttachmentWriteState);

			VkClearValue l_ClearColor{};
			l_ClearColor.color.float32[0] = 0.0008f;
			l_ClearColor.color.float32[1] = 0.0008f;
			l_ClearColor.color.float32[2] = 0.0008f;
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
		}

		ImDrawData* l_DrawData = ImGui::GetDrawData();
		if (l_DrawData != nullptr)
		{
			ImGui_ImplVulkan_RenderDrawData(l_DrawData, l_CommandBuffer);
			m_ImGuiVulkanInitialized = true;
		}
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

		const VulkanSwapchain::SceneColorPolicy& l_ColorPolicy = m_Swapchain.GetSceneColorPolicy();
		l_PushConstants.ColorInputTransfer = static_cast<uint32_t>(l_ColorPolicy.SceneInputTransfer);
		l_PushConstants.ColorOutputTransfer = static_cast<uint32_t>(l_ColorPolicy.SceneOutputTransfer);

		vkCmdPushConstants(l_CommandBuffer, m_Pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstants), &l_PushConstants);

		vkCmdDrawIndexed(l_CommandBuffer, a_GPUPrimitive.VulkanIB->GetIndexCount(), 1, 0, 0, 0);
	}

	void VulkanRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection)
	{
		DrawMesh(primitive, position, color, projection * view);
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

		if (m_Configuration.m_ColorOutputPolicy == VulkanSwapchain::ColorOutputPolicy::SDRsRGB && l_IsUnormSwapchain
			&& l_ColorPolicy.SceneOutputTransfer != ColorTransferMode::LinearToSrgb)
		{
			TR_CORE_CRITICAL("Scene color policy validation failed: UNORM swapchain requires explicit scene output conversion");

			std::abort();
		}

		if (m_Configuration.m_ColorOutputPolicy == VulkanSwapchain::ColorOutputPolicy::SDRsRGB && l_IsUnormSwapchain
			&& l_ColorPolicy.UiOutputTransfer != ColorTransferMode::LinearToSrgb)
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

	void VulkanRenderer::TransitionImageResource(VkCommandBuffer commandBuffer, VkImage image, const VkImageSubresourceRange& subresourceRange, const VulkanImageTransitionState& newState)
	{
		m_ResourceStateTracker.TransitionImageResource(commandBuffer, image, subresourceRange, newState);
	}
}