#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>

struct GLFWwindow;

namespace Engine
{
    class Window;
    class VulkanContext;
    class VulkanDevice;
    class VulkanRenderer;

    class VulkanSwapchain
    {
    public:
        VulkanSwapchain() = default;
        ~VulkanSwapchain() = default;

        VulkanSwapchain(const VulkanSwapchain&) = delete;
        VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
        VulkanSwapchain(VulkanSwapchain&&) = delete;
        VulkanSwapchain& operator=(VulkanSwapchain&&) = delete;

        void Initialize(VulkanContext& context, VulkanDevice& device, Window& window);
        void Shutdown();

        void Recreate(VulkanRenderer& renderer);
        bool IsValid() const { return m_Swapchain != VK_NULL_HANDLE; }
        explicit operator bool() const { return IsValid(); }

        VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
        VkFormat GetImageFormat() const { return m_ImageFormat; }
        VkExtent2D GetExtent() const { return m_Extent; }

        const std::vector<VkImage>& GetImages() const { return m_Images; }
        const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }

        // Convenience wrapper.
        VkResult AcquireNextImage(uint64_t timeout, VkSemaphore imageAvailableSemaphore, VkFence fence, uint32_t& outImageIndex) const;

    private:
        void Create(VkSwapchainKHR swapchain);
        void DestroySwapchainResources(const std::function<void(std::function<void()>&&)>& submitResourceFree = {});

        VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
        VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) const;
        VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    private:
        VulkanContext* m_Context = nullptr;
        VulkanDevice* m_Device = nullptr;
        Window* m_Window = nullptr;
        GLFWwindow* m_GLFWWindow = nullptr;

        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;

        VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_Extent{};
    };
}