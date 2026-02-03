#pragma once

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

		void Initialize(VkDevice device);
		void Create(uint32_t maxFramesInFlight, uint32_t swapchainImageCount);

		void Cleanup();

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
		void DestroySyncObjects();

	private:
		VkDevice m_Device = VK_NULL_HANDLE;

		uint32_t m_MaxFramesInFlight = 0;
		uint32_t m_SwapchainImageCount = 0;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkFence> m_InFlightFences;

		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_ImagesInFlight;

		bool m_IsCreated = false;
	};
}