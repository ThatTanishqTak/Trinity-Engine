#pragma once

#include <vector>

#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace Engine
{
    class VulkanSwapchain
    {
    public:
        struct SupportDetails
        {
            VkSurfaceCapabilitiesKHR Capabilities{};
            std::vector<VkSurfaceFormatKHR> Formats;
            std::vector<VkPresentModeKHR> PresentModes;
        };

    public:
        void Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window, uint32_t graphicsFamily, uint32_t presentFamily);
        void Shutdown();

        void Create();
        void Cleanup();

        VkSwapchainKHR GetHandle() const { return m_Swapchain; }
        VkFormat GetImageFormat() const { return m_ImageFormat; }
        VkExtent2D GetExtent() const { return m_Extent; }

        const std::vector<VkImage>& GetImages() const { return m_Images; }
        const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }
        uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }

        static SupportDetails QuerySupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    private:
        VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
        VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
        VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

        void CreateImageViews();

    private:
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        GLFWwindow* m_Window = nullptr;

        uint32_t m_GraphicsFamily = 0;
        uint32_t m_PresentFamily = 0;

        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;
        VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_Extent{};
    };
}