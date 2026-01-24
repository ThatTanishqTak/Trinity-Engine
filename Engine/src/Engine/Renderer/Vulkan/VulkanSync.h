#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Engine
{
    class VulkanDevice;

    class VulkanSync
    {
    public:
        VulkanSync() = default;
        ~VulkanSync() = default;

        VulkanSync(const VulkanSync&) = delete;
        VulkanSync& operator=(const VulkanSync&) = delete;
        VulkanSync(VulkanSync&&) = delete;
        VulkanSync& operator=(VulkanSync&&) = delete;

        void Initialize(VulkanDevice& device, uint32_t maxFramesInFlight, size_t swapchainImageCount);
        void Shutdown(VulkanDevice& device);

        // Call after swapchain recreate to reset per-image tracking.
        void OnSwapchainRecreated(size_t swapchainImageCount);

        VkSemaphore GetImageAvailableSemaphore(uint32_t frameIndex) const;
        VkSemaphore GetRenderFinishedSemaphore(uint32_t frameIndex) const;
        VkFence GetInFlightFence(uint32_t frameIndex) const;

        void WaitForFrameFence(VulkanDevice& device, uint32_t frameIndex, uint64_t timeout = UINT64_MAX) const;
        void ResetFrameFence(VulkanDevice& device, uint32_t frameIndex) const;

        void WaitForImageFenceIfNeeded(VulkanDevice& device, uint32_t imageIndex, uint64_t timeout = UINT64_MAX) const;
        void MarkImageInFlight(uint32_t imageIndex, VkFence owningFrameFence);

        bool IsValid() const;

    private:
        uint32_t m_MaxFramesInFlight = 0;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_InFlightFences;

        // One fence per swapchain image, tracking which frame fence currently owns it.
        std::vector<VkFence> m_ImagesInFlight;

        bool m_Initialized = false;
    };
}