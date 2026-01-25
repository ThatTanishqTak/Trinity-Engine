#include "Engine/Renderer/Vulkan/VulkanSwapchain.h"

#include "Engine/Platform/Window.h"
#include "Engine/Renderer/Vulkan/VulkanContext.h"
#include "Engine/Renderer/Vulkan/VulkanDevice.h"
#include "Engine/Renderer/Vulkan/VulkanResources.h"
#include "Engine/Utilities/Utilities.h"

#include <GLFW/glfw3.h>

#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace Engine
{
    void VulkanSwapchain::Initialize(VulkanContext& context, VulkanDevice& device, Window& window)
    {
        m_Context = &context;
        m_Device = &device;
        m_Window = &window;
        m_GLFWWindow = (GLFWwindow*)window.GetNativeWindow();

        Create(VK_NULL_HANDLE);

        TR_CORE_INFO("VulkanSwapchain initialized ({} images, {}x{})", (uint32_t)m_Images.size(), m_Extent.width, m_Extent.height);
    }

    void VulkanSwapchain::Shutdown()
    {
        if (m_Device && m_Device->GetDevice())
        {
            DestroySwapchainResources();
        }

        m_Context = nullptr;
        m_Device = nullptr;
        m_Window = nullptr;
        m_GLFWWindow = nullptr;

        m_ImageFormat = VK_FORMAT_UNDEFINED;
        m_Extent = {};
    }

    void VulkanSwapchain::Recreate()
    {
        if (!m_Device || !m_Device->GetDevice() || !m_Context || !m_Context->GetSurface())
        {
            return;
        }

        // Window minimized: wait until it has a valid framebuffer size
        int l_Width = 0;
        int l_Height = 0;
        if (m_GLFWWindow)
        {
            glfwGetFramebufferSize(m_GLFWWindow, &l_Width, &l_Height);
            while (l_Width == 0 || l_Height == 0)
            {
                glfwWaitEvents();
                glfwGetFramebufferSize(m_GLFWWindow, &l_Width, &l_Height);
            }
        }

        VkSwapchainKHR l_OldSwapchain = m_Swapchain;

        // Preserve old views so we can destroy them after successful recreate
        std::vector<VkImageView> l_OldImageViews = std::move(m_ImageViews);
        m_ImageViews.clear();
        m_Images.clear();

        Create(l_OldSwapchain);

        // Destroy old views and old swapchain (new one is now active)
        for (auto it_ImageView : l_OldImageViews)
        {
            if (it_ImageView)
            {
                VulkanResources::DestroyImageView(*m_Device, it_ImageView);
            }
        }

        if (l_OldSwapchain)
        {
            vkDestroySwapchainKHR(m_Device->GetDevice(), l_OldSwapchain, nullptr);
        }

        TR_CORE_INFO("VulkanSwapchain recreated ({} images, {}x{})", (uint32_t)m_Images.size(), m_Extent.width, m_Extent.height);
    }

    VkResult VulkanSwapchain::AcquireNextImage(uint64_t timeout, VkSemaphore imageAvailableSemaphore, VkFence fence, uint32_t& outImageIndex) const
    {
        if (!m_Device || !m_Device->GetDevice() || !m_Swapchain)
        {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        return vkAcquireNextImageKHR(m_Device->GetDevice(), m_Swapchain, timeout, imageAvailableSemaphore, fence, &outImageIndex);
    }

    void VulkanSwapchain::Create(VkSwapchainKHR swpachain)
    {
        const auto a_Support = m_Device->QuerySwapchainSupport(m_Device->GetPhysicalDevice());
        if (a_Support.Formats.empty() || a_Support.PresentModes.empty())
        {
            TR_CORE_CRITICAL("Swapchain support is incomplete (no formats or present modes)");

            std::abort;
        }

        const auto a_SurfaceFormat = ChooseSurfaceFormat(a_Support.Formats);
        const auto a_PresentMode = ChoosePresentMode(a_Support.PresentModes);
        const auto a_Extent = ChooseExtent(a_Support.Capabilities);

        uint32_t l_ImageCount = a_Support.Capabilities.minImageCount + 1;
        if (a_Support.Capabilities.maxImageCount > 0 && l_ImageCount > a_Support.Capabilities.maxImageCount)
        {
            l_ImageCount = a_Support.Capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR l_SwapchainCreateInfo{};
        l_SwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        l_SwapchainCreateInfo.surface = m_Context->GetSurface();
        l_SwapchainCreateInfo.minImageCount = l_ImageCount;
        l_SwapchainCreateInfo.imageFormat = a_SurfaceFormat.format;
        l_SwapchainCreateInfo.imageColorSpace = a_SurfaceFormat.colorSpace;
        l_SwapchainCreateInfo.imageExtent = a_Extent;
        l_SwapchainCreateInfo.imageArrayLayers = 1;
        l_SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const uint32_t l_QueueFamilyIndices[] = { m_Device->GetGraphicsQueueFamily(), m_Device->GetPresentQueueFamily() };

        if (m_Device->GetGraphicsQueueFamily() != m_Device->GetPresentQueueFamily())
        {
            l_SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            l_SwapchainCreateInfo.queueFamilyIndexCount = 2;
            l_SwapchainCreateInfo.pQueueFamilyIndices = l_QueueFamilyIndices;
        }
        else
        {
            l_SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            l_SwapchainCreateInfo.queueFamilyIndexCount = 0;
            l_SwapchainCreateInfo.pQueueFamilyIndices = nullptr;
        }

        l_SwapchainCreateInfo.preTransform = a_Support.Capabilities.currentTransform;
        l_SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        l_SwapchainCreateInfo.presentMode = a_PresentMode;
        l_SwapchainCreateInfo.clipped = VK_TRUE;
        l_SwapchainCreateInfo.oldSwapchain = swpachain;

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateSwapchainKHR(m_Device->GetDevice(), &l_SwapchainCreateInfo, nullptr, &m_Swapchain), "vkCreateSwapchainKHR");

        // Fetch swapchain images.
        uint32_t l_ActualImageCount = 0;
        Utilities::VulkanUtilities::VKCheckStrict(vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &l_ActualImageCount, nullptr), "vkGetSwapchainImagesKHR (count)");

        m_Images.resize(l_ActualImageCount);
        Utilities::VulkanUtilities::VKCheckStrict(vkGetSwapchainImagesKHR(m_Device->GetDevice(), m_Swapchain, &l_ActualImageCount, m_Images.data()), "vkGetSwapchainImagesKHR (data)");

        m_ImageFormat = a_SurfaceFormat.format;
        m_Extent = a_Extent;

        // Create image views.
        m_ImageViews.resize(m_Images.size());
        for (size_t i = 0; i < m_Images.size(); ++i)
        {
            // Swapchain image views must exist so the render pass can attach them.
            m_ImageViews[i] = VulkanResources::CreateImageView(*m_Device, m_Images[i], m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void VulkanSwapchain::DestroySwapchainResources()
    {
        if (!m_Device || !m_Device->GetDevice())
        {
            return;
        }

        for (auto it_ImageView : m_ImageViews)
        {
            if (it_ImageView)
            {
                vkDestroyImageView(m_Device->GetDevice(), it_ImageView, nullptr);
            }
        }
        m_ImageViews.clear();

        if (m_Swapchain)
        {
            vkDestroySwapchainKHR(m_Device->GetDevice(), m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }

        m_Images.clear();
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

        return formats[0];
    }

    VkPresentModeKHR VulkanSwapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) const
    {
        const bool l_VSync = m_Window ? m_Window->IsVSync() : true;
        if (l_VSync)
        {
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        for (auto it_Mode : modes)
        {
            if (it_Mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return it_Mode;
            }
        }

        for (auto it_Mode : modes)
        {
            if (it_Mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
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

        if (m_GLFWWindow)
        {
            glfwGetFramebufferSize(m_GLFWWindow, &l_Width, &l_Height);
        }

        VkExtent2D l_Extent{};
        l_Extent.width = (uint32_t)std::clamp(l_Width, (int)capabilities.minImageExtent.width, (int)capabilities.maxImageExtent.width);
        l_Extent.height = (uint32_t)std::clamp(l_Height, (int)capabilities.minImageExtent.height, (int)capabilities.maxImageExtent.height);

        return l_Extent;
    }
}