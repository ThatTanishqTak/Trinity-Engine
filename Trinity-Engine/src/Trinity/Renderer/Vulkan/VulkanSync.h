#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanContext;
	class VulkanDevice;

	class VulkanSync
	{
	public:
		void Initialize(const VulkanContext& context, const VulkanDevice& device, uint32_t framesInFlight, uint32_t swapchainImageCount);
		void Shutdown();

		void OnSwapchainRecreated(uint32_t swapchainImageCount);

		uint32_t GetFramesInFlight() const { return m_FramesInFlight; }
		uint32_t GetSwapchainImageCount() const { return m_SwapchainImageCount; }

		VkSemaphore GetImageAvailableSemaphore(uint32_t frameIndex) const;
		VkSemaphore GetRenderFinishedSemaphore(uint32_t frameIndex) const;
		VkFence GetInFlightFence(uint32_t frameIndex) const;

		void WaitForFrameFence(uint32_t frameIndex, uint64_t timeout = UINT64_MAX) const;
		void ResetFrameFence(uint32_t frameIndex) const;

		VkFence GetImageInFlightFence(uint32_t imageIndex) const;
		void SetImageInFlightFence(uint32_t imageIndex, VkFence fence);
		void ClearImagesInFlight();

	private:
		void DestroySyncObjects();
		void ValidateFrameIndex(uint32_t frameIndex) const;
		void ValidateImageIndex(uint32_t imageIndex) const;

	private:
		VkAllocationCallbacks* m_Allocator = nullptr;
		VkDevice m_Device = VK_NULL_HANDLE;

		uint32_t m_FramesInFlight = 0;
		uint32_t m_SwapchainImageCount = 0;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores{};
		std::vector<VkSemaphore> m_RenderFinishedSemaphores{};
		std::vector<VkFence> m_InFlightFences{};

		std::vector<VkFence> m_ImagesInFlight{};
	};
}