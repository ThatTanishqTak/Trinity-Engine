#pragma once

#include "Trinity/Renderer/Renderer.h"

#include "Trinity/Renderer/Vulkan/VulkanInstance.h"
#include "Trinity/Renderer/Vulkan/VulkanSurface.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"
#include "Trinity/Renderer/Vulkan/VulkanRenderPass.h"
#include "Trinity/Renderer/Vulkan/VulkanFramebuffers.h"
#include "Trinity/Renderer/Vulkan/VulkanFrameResources.h"
#include "Trinity/Renderer/Vulkan/VulkanDescriptors.h"
#include "Trinity/Renderer/Vulkan/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/VulkanCommand.h"
#include "Trinity/Renderer/Vulkan/VulkanSync.h"
#include "Trinity/Geometry/Mesh.h"

#include <optional>

namespace Trinity
{
	class Window;

	class VulkanRenderer : public Renderer
	{
	private:
		struct FrameContext
		{
			uint32_t FrameIndex = 0;
			uint32_t ImageIndex = 0;
			VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
			VkSemaphore ImageAvailableSemaphore = VK_NULL_HANDLE;
			VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;
			VkFence InFlightFence = VK_NULL_HANDLE;
		};

	public:
		VulkanRenderer();
		~VulkanRenderer();

		void SetWindow(Window& window) override;

		void Initialize() override;
		void Shutdown() override;

		void Resize(uint32_t width, uint32_t height) override;

		void BeginFrame() override;
		void EndFrame() override;

	private:
		void RecreateSwapchain(uint32_t preferredWidth, uint32_t preferredHeight);

	private:
		Window* m_Window = nullptr;

		VulkanInstance m_Instance;
		VulkanSurface m_Surface;
		VulkanDevice m_Device;
		VulkanContext m_Context;
		VulkanSwapchain m_Swapchain;
		VulkanRenderPass m_RenderPass;
		VulkanFramebuffers m_Framebuffers;
		VulkanFrameResources m_FrameResources;
		VulkanDescriptors m_Descriptors;
		VulkanPipeline m_Pipeline;
		VulkanCommand m_Command;
		VulkanSync m_Sync;
		Geometry::Mesh m_Mesh;

		uint32_t m_FramesInFlight = 2;
		std::optional<FrameContext> m_FrameContext;
		bool m_FrameBegun = false;

		bool m_ResizePending = false;
		uint32_t m_PendingWidth = 0;
		uint32_t m_PendingHeight = 0;
	};
}