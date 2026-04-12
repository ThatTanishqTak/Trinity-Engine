#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanSyncObjects
	{
	public:
		VulkanSyncObjects() = default;
		~VulkanSyncObjects() = default;

		void Initialize(VkDevice device, uint32_t framesInFlight);
		void Shutdown();

		VkSemaphore GetImageAvailableSemaphore(uint32_t frameIndex) const { return m_ImageAvailableSemaphores[frameIndex]; }
		VkSemaphore GetRenderFinishedSemaphore(uint32_t frameIndex) const { return m_RenderFinishedSemaphores[frameIndex]; }
		VkFence GetInFlightFence(uint32_t frameIndex) const { return m_InFlightFences[frameIndex]; }

		void WaitForFence(uint32_t frameIndex) const;
		void ResetFence(uint32_t frameIndex) const;

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
	};
}