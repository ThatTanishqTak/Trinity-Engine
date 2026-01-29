#include "Engine/Renderer/Vulkan/VulkanSwapchain.h"

#include "Engine/Utilities/Utilities.h"

#include <GLFW/glfw3.h>

#include <algorithm>

namespace Engine
{
    void VulkanSwapchain::Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window, uint32_t graphicsFamily, uint32_t presentFamily)
    {
        m_PhysicalDevice = physicalDevice;
        m_Device = device;
        m_Surface = surface;
        m_Window = window;
        m_GraphicsFamily = graphicsFamily;
        m_PresentFamily = presentFamily;
    }

    void VulkanSwapchain::Shutdown()
    {
        Cleanup();

        m_PhysicalDevice = VK_NULL_HANDLE;
        m_Device = VK_NULL_HANDLE;
        m_Surface = VK_NULL_HANDLE;
        m_Window = nullptr;
        m_GraphicsFamily = 0;
        m_PresentFamily = 0;
    }

    VulkanSwapchain::SupportDetails VulkanSwapchain::QuerySupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SupportDetails l_Details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &l_Details.Capabilities);

        uint32_t l_FormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &l_FormatCount, nullptr);
        if (l_FormatCount > 0)
        {
            l_Details.Formats.resize(l_FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &l_FormatCount, l_Details.Formats.data());
        }

        uint32_t l_PresentCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &l_PresentCount, nullptr);
        if (l_PresentCount > 0)
        {
            l_Details.PresentModes.resize(l_PresentCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &l_PresentCount, l_Details.PresentModes.data());
        }

        return l_Details;
    }

    VkSurfaceFormatKHR VulkanSwapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
    {
        for (const auto& it_Format : formats)
        {
            if (it_Format.format == VK_FORMAT_B8G8R8A8_SRGB && it_Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return it_Format;
            }
        }

        return formats.empty() ? VkSurfaceFormatKHR{} : formats[0];
    }

    VkPresentModeKHR VulkanSwapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
    {
        for (const auto& it_Mode : presentModes)
        {
            if (it_Mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return it_Mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }

        int l_Width = 0;
        int l_Height = 0;
        glfwGetFramebufferSize(m_Window, &l_Width, &l_Height);

        VkExtent2D l_Extent{};
        l_Extent.width = static_cast<uint32_t>(std::clamp(l_Width, static_cast<int>(capabilities.minImageExtent.width), static_cast<int>(capabilities.maxImageExtent.width)));
        l_Extent.height = static_cast<uint32_t>(std::clamp(l_Height, static_cast<int>(capabilities.minImageExtent.height), static_cast<int>(capabilities.maxImageExtent.height)));

        return l_Extent;
    }

    VkFormat VulkanSwapchain::FindDepthFormat() const
    {
        const VkFormat l_Candidates[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };

        for (VkFormat it_Format : l_Candidates)
        {
            VkFormatProperties l_Properties{};
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, it_Format, &l_Properties);
            if ((l_Properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
            {
                return it_Format;
            }
        }

        return VK_FORMAT_UNDEFINED;
    }

    void VulkanSwapchain::Create()
    {
        const auto a_Support = QuerySupport(m_PhysicalDevice, m_Surface);

        const VkSurfaceFormatKHR l_SurfaceFormat = ChooseSurfaceFormat(a_Support.Formats);
        const VkPresentModeKHR l_PresentMode = ChoosePresentMode(a_Support.PresentModes);
        const VkExtent2D l_Extent = ChooseExtent(a_Support.Capabilities);

        uint32_t l_ImageCount = a_Support.Capabilities.minImageCount + 1;
        if (a_Support.Capabilities.maxImageCount > 0 && l_ImageCount > a_Support.Capabilities.maxImageCount)
        {
            l_ImageCount = a_Support.Capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        l_CreateInfo.surface = m_Surface;
        l_CreateInfo.minImageCount = l_ImageCount;
        l_CreateInfo.imageFormat = l_SurfaceFormat.format;
        l_CreateInfo.imageColorSpace = l_SurfaceFormat.colorSpace;
        l_CreateInfo.imageExtent = l_Extent;
        l_CreateInfo.imageArrayLayers = 1;
        l_CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const uint32_t l_QueueFamilyIndices[] = { m_GraphicsFamily, m_PresentFamily };

        if (m_GraphicsFamily != m_PresentFamily)
        {
            l_CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            l_CreateInfo.queueFamilyIndexCount = 2;
            l_CreateInfo.pQueueFamilyIndices = l_QueueFamilyIndices;
        }
        else
        {
            l_CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        l_CreateInfo.preTransform = a_Support.Capabilities.currentTransform;
        l_CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        l_CreateInfo.presentMode = l_PresentMode;
        l_CreateInfo.clipped = VK_TRUE;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateSwapchainKHR(m_Device, &l_CreateInfo, nullptr, &m_Swapchain), "vkCreateSwapchainKHR");

        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &l_ImageCount, nullptr);
        m_Images.resize(l_ImageCount);
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &l_ImageCount, m_Images.data());

        m_ImageFormat = l_SurfaceFormat.format;
        m_Extent = l_Extent;

        CreateImageViews();
        CreateDepthResources();
    }

    void VulkanSwapchain::CreateImageViews()
    {
        m_ImageViews.resize(m_Images.size(), VK_NULL_HANDLE);

        for (size_t i = 0; i < m_Images.size(); ++i)
        {
            VkImageViewCreateInfo l_ViewInfo{};
            l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            l_ViewInfo.image = m_Images[i];
            l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            l_ViewInfo.format = m_ImageFormat;
            l_ViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            l_ViewInfo.subresourceRange.baseMipLevel = 0;
            l_ViewInfo.subresourceRange.levelCount = 1;
            l_ViewInfo.subresourceRange.baseArrayLayer = 0;
            l_ViewInfo.subresourceRange.layerCount = 1;

            Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateImageView(m_Device, &l_ViewInfo, nullptr, &m_ImageViews[i]), "vkCreateImageView");
        }
    }

    void VulkanSwapchain::CreateDepthResources()
    {
        CleanupDepthResources();

        const VkFormat l_DepthFormat = FindDepthFormat();
        if (l_DepthFormat == VK_FORMAT_UNDEFINED)
        {
            TR_CORE_CRITICAL("VulkanSwapchain failed to find a supported depth format.");

            return;
        }

        CreateImage(m_Extent.width, m_Extent.height, l_DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_DepthImage, m_DepthMemory);

        VkImageAspectFlags l_AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (HasStencilComponent(l_DepthFormat))
        {
            l_AspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        m_DepthView = CreateImageView(m_DepthImage, l_DepthFormat, l_AspectFlags);
        m_DepthFormat = l_DepthFormat;
    }

    void VulkanSwapchain::CleanupDepthResources()
    {
        if (m_DepthView)
        {
            vkDestroyImageView(m_Device, m_DepthView, nullptr);
            m_DepthView = VK_NULL_HANDLE;
        }

        if (m_DepthImage)
        {
            vkDestroyImage(m_Device, m_DepthImage, nullptr);
            m_DepthImage = VK_NULL_HANDLE;
        }

        if (m_DepthMemory)
        {
            vkFreeMemory(m_Device, m_DepthMemory, nullptr);
            m_DepthMemory = VK_NULL_HANDLE;
        }

        m_DepthFormat = VK_FORMAT_UNDEFINED;
    }

    bool VulkanSwapchain::HasStencilComponent(VkFormat format) const
    {
        return format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    uint32_t VulkanSwapchain::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDeviceMemoryProperties l_MemoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &l_MemoryProperties);

        for (uint32_t i = 0; i < l_MemoryProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) && (l_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        TR_CORE_CRITICAL("Failed to find suitable memory type.");

        return 0;
    }

    void VulkanSwapchain::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& outImage, VkDeviceMemory& outMemory) const
    {
        outImage = VK_NULL_HANDLE;
        outMemory = VK_NULL_HANDLE;

        VkImageCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        l_Info.imageType = VK_IMAGE_TYPE_2D;
        l_Info.extent.width = width;
        l_Info.extent.height = height;
        l_Info.extent.depth = 1;
        l_Info.mipLevels = 1;
        l_Info.arrayLayers = 1;
        l_Info.format = format;
        l_Info.tiling = tiling;
        l_Info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        l_Info.usage = usage;
        l_Info.samples = VK_SAMPLE_COUNT_1_BIT;
        l_Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateImage(m_Device, &l_Info, nullptr, &outImage), "vkCreateImage");

        VkMemoryRequirements l_MemReq{};
        vkGetImageMemoryRequirements(m_Device, outImage, &l_MemReq);

        VkMemoryAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        l_AllocInfo.allocationSize = l_MemReq.size;
        l_AllocInfo.memoryTypeIndex = FindMemoryType(l_MemReq.memoryTypeBits, properties);

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkAllocateMemory(m_Device, &l_AllocInfo, nullptr, &outMemory), "vkAllocateMemory");
        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkBindImageMemory(m_Device, outImage, outMemory, 0), "vkBindImageMemory");
    }

    VkImageView VulkanSwapchain::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const
    {
        VkImageViewCreateInfo l_View{};
        l_View.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_View.image = image;
        l_View.viewType = VK_IMAGE_VIEW_TYPE_2D;
        l_View.format = format;
        l_View.subresourceRange.aspectMask = aspectFlags;
        l_View.subresourceRange.baseMipLevel = 0;
        l_View.subresourceRange.levelCount = 1;
        l_View.subresourceRange.baseArrayLayer = 0;
        l_View.subresourceRange.layerCount = 1;

        VkImageView l_ImageView = VK_NULL_HANDLE;
        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateImageView(m_Device, &l_View, nullptr, &l_ImageView), "vkCreateImageView");

        return l_ImageView;
    }

    void VulkanSwapchain::Cleanup()
    {
        CleanupDepthResources();

        for (VkImageView it_View : m_ImageViews)
        {
            if (it_View)
            {
                vkDestroyImageView(m_Device, it_View, nullptr);
            }
        }

        m_ImageViews.clear();
        m_Images.clear();

        if (m_Swapchain)
        {
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }

        m_ImageFormat = VK_FORMAT_UNDEFINED;
        m_Extent = {};
    }
}