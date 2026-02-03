#pragma once

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Vulkan/VulkanInstance.h"
#include "Trinity/Renderer/Vulkan/VulkanSurface.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"
#include "Trinity/Renderer/Vulkan/VulkanRenderPass.h"
#include "Trinity/Renderer/Vulkan/VulkanFramebuffers.h"
#include "Trinity/Renderer/Vulkan/VulkanFrameResources.h"
#include "Trinity/Renderer/Vulkan/VulkanDescriptors.h"
#include "Trinity/Renderer/Vulkan/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/VulkanCommand.h"
#include "Trinity/Renderer/Vulkan/VulkanSync.h"

namespace Trinity
{
	class Window;

	class VulkanRenderer : public Renderer
	{
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
		VulkanSwapchain m_Swapchain;
		VulkanRenderPass m_RenderPass;
		VulkanFramebuffers m_Framebuffers;
		VulkanFrameResources m_FrameResources;
		VulkanDescriptors m_Descriptors;
		VulkanPipeline m_Pipeline;
		VulkanCommand m_Command;
		VulkanSync m_Sync;

		uint32_t m_FramesInFlight = 2;
		uint32_t m_CurrentImageIndex = 0;
		bool m_FrameBegun = false;

		bool m_ResizePending = false;
		uint32_t m_PendingWidth = 0;
		uint32_t m_PendingHeight = 0;
	};
}