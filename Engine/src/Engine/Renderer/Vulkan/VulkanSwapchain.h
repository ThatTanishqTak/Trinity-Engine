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
        VkFormat GetDepthFormat() const { return m_DepthFormat; }
        VkImageView GetDepthView() const { return m_DepthView; }

        const std::vector<VkImage>& GetImages() const { return m_Images; }
        const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }
        uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }

        static SupportDetails QuerySupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    private:
        VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
        VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
        VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
        VkFormat FindDepthFormat() const;

        void CreateImageViews();
        void CreateDepthResources();
        void CleanupDepthResources();
        bool HasStencilComponent(VkFormat format) const;

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& outImage,
            VkDeviceMemory& outMemory) const;
        VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;

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

        VkImage m_DepthImage = VK_NULL_HANDLE;
        VkDeviceMemory m_DepthMemory = VK_NULL_HANDLE;
        VkImageView m_DepthView = VK_NULL_HANDLE;
        VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
    };
}