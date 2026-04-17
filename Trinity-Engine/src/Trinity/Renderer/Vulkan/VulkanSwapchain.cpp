#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"

#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

#include <algorithm>
#include <limits>

namespace Trinity
{
    void VulkanSwapchain::Initialize(VulkanDevice& device, uint32_t width, uint32_t height, bool vsync)
    {
        m_Device = &device;
        m_VSync = vsync;

        CreateSwapchain(width, height);
        CreateImageViews();

        TR_CORE_INFO("Vulkan swapchain created ({}x{}, {} images)", m_Extent.width, m_Extent.height, m_Images.size());
    }

    void VulkanSwapchain::Shutdown()
    {
        if (m_Device == nullptr)
        {
            return;
        }

        CleanupSwapchain();
        m_Device = nullptr;
    }

    void VulkanSwapchain::Recreate(uint32_t width, uint32_t height)
    {
        vkDeviceWaitIdle(m_Device->GetDevice());

        CleanupSwapchain();
        CreateSwapchain(width, height);
        CreateImageViews();

        TR_CORE_DEBUG("Swapchain recreated ({}x{})", m_Extent.width, m_Extent.height);
    }

    void VulkanSwapchain::CreateSwapchain(uint32_t width, uint32_t height)
    {
        SwapchainSupportDetails l_Support = m_Device->QuerySwapchainSupport();

        VkSurfaceFormatKHR l_SurfaceFormat = ChooseSurfaceFormat(l_Support.Formats);
        VkPresentModeKHR l_PresentMode = ChoosePresentMode(l_Support.PresentModes);
        VkExtent2D l_Extent = ChooseExtent(l_Support.Capabilities, width, height);

        uint32_t l_ImageCount = l_Support.Capabilities.minImageCount + 1;
        if (l_Support.Capabilities.maxImageCount > 0 && l_ImageCount > l_Support.Capabilities.maxImageCount)
        {
            l_ImageCount = l_Support.Capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        l_CreateInfo.surface = m_Device->GetSurface();
        l_CreateInfo.minImageCount = l_ImageCount;
        l_CreateInfo.imageFormat = l_SurfaceFormat.format;
        l_CreateInfo.imageColorSpace = l_SurfaceFormat.colorSpace;
        l_CreateInfo.imageExtent = l_Extent;
        l_CreateInfo.imageArrayLayers = 1;
        l_CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        const QueueFamilyIndices& l_Indices = m_Device->GetQueueFamilyIndices();
        uint32_t l_QueueFamilyIndices[] = { l_Indices.GraphicsFamily.value(), l_Indices.PresentFamily.value() };

        if (l_Indices.GraphicsFamily != l_Indices.PresentFamily)
        {
            l_CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            l_CreateInfo.queueFamilyIndexCount = 2;
            l_CreateInfo.pQueueFamilyIndices = l_QueueFamilyIndices;
        }
        else
        {
            l_CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        l_CreateInfo.preTransform = l_Support.Capabilities.currentTransform;
        l_CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        l_CreateInfo.presentMode = l_PresentMode;
        l_CreateInfo.clipped = VK_TRUE;
        l_CreateInfo.oldSwapchain = VK_NULL_HANDLE;

        VulkanUtilities::VKCheck(vkCreateSwapchainKHR(m_Device->GetDevice(), &l_CreateInfo, nullptr, &m_Swapchain), "Failed vkCreateSwapchainKHR");

        m_ImageFormat = l_SurfaceFormat.format;
        m_Extent = l_Extent;

        vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &l_ImageCount, nullptr);
        m_Images.resize(l_ImageCount);
        vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &l_ImageCount, m_Images.data());
    }

    void VulkanSwapchain::CreateImageViews()
    {
        m_ImageViews.resize(m_Images.size());

        for (size_t i = 0; i < m_Images.size(); i++)
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

            VulkanUtilities::VKCheck(vkCreateImageView(m_Device->GetDevice(), &l_ViewInfo, nullptr, &m_ImageViews[i]), "Failed vkCreateImageView");
        }
    }

    void VulkanSwapchain::CleanupSwapchain()
    {
        VkDevice l_Device = m_Device->GetDevice();

        for (auto l_ImageView : m_ImageViews)
        {
            vkDestroyImageView(l_Device, l_ImageView, nullptr);
        }

        m_ImageViews.clear();
        m_Images.clear();

        if (m_Swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(l_Device, m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }
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

        for (const auto& it_Format : formats)
        {
            if (it_Format.format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                return it_Format;
            }
        }

        return formats[0];
    }

    VkPresentModeKHR VulkanSwapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
    {
        if (!m_VSync)
        {
            for (const auto& it_Mode : presentModes)
            {
                if (it_Mode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    return it_Mode;
                }
            }

            for (const auto& it_Mode : presentModes)
            {
                if (it_Mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                    return it_Mode;
                }
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) const
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }

        VkExtent2D l_ActualExtent = { width, height };
        l_ActualExtent.width = std::clamp(l_ActualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        l_ActualExtent.height = std::clamp(l_ActualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return l_ActualExtent;
    }
}