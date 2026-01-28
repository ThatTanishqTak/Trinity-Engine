#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace Engine
{
    class VulkanSync
    {
    public:
        void Initialize(VkDevice device, uint32_t maxFramesInFlight);
        void Shutdown();

        void CreatePerFrame();
        void DestroyPerFrame();

        void RecreatePerImage(uint32_t swapchainImageCount);
        void DestroyPerImage();

        VkSemaphore GetImageAvailableSemaphore(uint32_t frameIndex) const;
        VkSemaphore GetRenderFinishedSemaphore(uint32_t imageIndex) const;
        VkFence GetInFlightFence(uint32_t frameIndex) const;

        void WaitForFrame(uint32_t frameIndex) const;
        void ResetFrame(uint32_t frameIndex) const;

        void WaitForImage(uint32_t imageIndex) const;
        void SetImageInFlight(uint32_t imageIndex, VkFence fence);

        uint32_t GetMaxFramesInFlight() const { return m_MaxFramesInFlight; }
        uint32_t GetSwapchainImageCount() const { return static_cast<uint32_t>(m_RenderFinishedSemaphores.size()); }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        uint32_t m_MaxFramesInFlight = 0;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkFence> m_InFlightFences;

        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_ImagesInFlight;
    };
}