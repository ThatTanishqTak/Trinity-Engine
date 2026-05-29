#pragma once

#include <vector>
#include <cstdint>

#include <vulkan/vulkan.h>

#include <Trinity/Renderer/RHI/Swapchain.h>

namespace Trinity
{
    class VulkanDevice;

    struct VulkanSwapchainSupport
    {
        VkSurfaceCapabilitiesKHR Capabilities{};
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    struct VulkanFrameData
    {
        VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
        VkSemaphore ImageAvailable = VK_NULL_HANDLE;
        VkFence InFlight = VK_NULL_HANDLE;
    };

    class VulkanSwapchain : public Swapchain
    {
    public:
        static constexpr uint32_t MaxFramesInFlight = 2;

        VulkanSwapchain(VulkanDevice& device, const SwapchainDescription& description);
        ~VulkanSwapchain() override;

        VulkanSwapchain(const VulkanSwapchain&) = delete;
        VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

        bool Initialize();
        void Shutdown();

        bool AcquireNextImage(FrameInfo& outFrame) override;
        void Present() override;
        void Resize(uint32_t width, uint32_t height) override;
        void RenderFrame(float red, float green, float blue);

        uint32_t GetWidth() const override { return m_Extent.width; }
        uint32_t GetHeight() const override { return m_Extent.height; }
        Format GetFormat() const override { return m_FrontendFormat; }
        uint32_t GetImageCount() const override { return static_cast<uint32_t>(m_Images.size()); }

        VkSwapchainKHR GetHandle() const { return m_Swapchain; }
        VkFormat GetVulkanFormat() const { return m_SurfaceFormat.format; }
        const std::vector<VkImage>& GetImages() const { return m_Images; }
        const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }

    private:
        bool CreateSwapchain();
        bool CreateImageViews();
        bool CreateFrameData();
        void DestroySwapchainObjects();
        void DestroyFrameData();
        void TransitionImageLayout(VkCommandBuffer command, VkImage image, VkImageLayout from, VkImageLayout to);

        VulkanSwapchainSupport QuerySupport() const;
        VkSurfaceFormatKHR ChooseFormat(const std::vector<VkSurfaceFormatKHR>& available) const;
        VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& available) const;
        VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    private:
        VulkanDevice& m_Device;
        SwapchainDescription m_Description;

        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR m_SurfaceFormat{};
        VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
        VkExtent2D m_Extent{ 0, 0 };
        Format m_FrontendFormat = Format::Unknown;

        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;

        std::vector<VulkanFrameData> m_Frames;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        uint32_t m_CurrentFrame = 0;
        uint32_t m_CurrentImageIndex = 0;
    };
}