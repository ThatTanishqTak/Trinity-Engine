#pragma once

#include "Trinity/Renderer/Vulkan/VulkanContext.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanSync
	{
	public:
		VulkanSync() = default;
		~VulkanSync() = default;

		VulkanSync(const VulkanSync&) = delete;
		VulkanSync& operator=(const VulkanSync&) = delete;
		VulkanSync(VulkanSync&&) = delete;
		VulkanSync& operator=(VulkanSync&&) = delete;

		void Initialize(const VulkanContext& context, uint32_t maxFramesInFlight, uint32_t swapchainImageCount);
		void Shutdown();

		void RecreateForSwapchain(uint32_t swapchainImageCount);

	public:
		VkSemaphore GetImageAvailableSemaphore(uint32_t frameIndex) const;
		VkSemaphore GetRenderFinishedSemaphore(uint32_t imageIndex) const;

		VkFence GetInFlightFence(uint32_t frameIndex) const;

		VkFence GetImageInFlightFence(uint32_t imageIndex) const;
		void SetImageInFlightFence(uint32_t imageIndex, VkFence fence);

		void WaitForFrameFence(uint32_t frameIndex, uint64_t timeout = UINT64_MAX) const;
		void ResetFrameFence(uint32_t frameIndex) const;

		uint32_t GetMaxFramesInFlight() const { return m_MaxFramesInFlight; }
		uint32_t GetSwapchainImageCount() const { return m_SwapchainImageCount; }

	private:
		void CreateFrameSyncObjects();
		void DestroyFrameSyncObjects();

		void CreateSwapchainSyncObjects(uint32_t swapchainImageCount);
		void DestroySwapchainSyncObjects();

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;

		uint32_t m_MaxFramesInFlight = 0;
		uint32_t m_SwapchainImageCount = 0;

		// Per-frame
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkFence> m_InFlightFences;

		// Per-swapchain-image (avoids semaphore reuse warnings)
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;

		// Fence tracking per swapchain image
		std::vector<VkFence> m_ImagesInFlight;
	};
}