#include <Trinity/Renderer/Backends/Vulkan/VulkanSwapchain.h>

#include <algorithm>
#include <set>

#include <Trinity/Core/Log.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanDevice.h>

namespace Trinity
{
    static Format VulkanFormatToFrontend(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R8G8B8A8_UNORM: return Format::RGBA8_UNORM;
            case VK_FORMAT_R8G8B8A8_SRGB: return Format::RGBA8_SRGB;
            case VK_FORMAT_B8G8R8A8_UNORM: return Format::BGRA8_UNORM;
            case VK_FORMAT_B8G8R8A8_SRGB: return Format::BGRA8_SRGB;
            default: return Format::Unknown;
        }
    }

    static VkFormat FrontendFormatToVulkan(Format format)
    {
        switch (format)
        {
            case Format::RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
            case Format::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
            case Format::BGRA8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
            case Format::BGRA8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
            default: return VK_FORMAT_UNDEFINED;
        }
    }

    VulkanSwapchain::VulkanSwapchain(VulkanDevice& device, const SwapchainDescription& description) : m_Device(device), m_Description(description)
    {

    }

    VulkanSwapchain::~VulkanSwapchain()
    {
        Shutdown();
    }

    bool VulkanSwapchain::Initialize()
    {
        if (!CreateSwapchain())
        {
            return false;
        }

        if (!CreateImageViews())
        {
            return false;
        }

        if (!CreateFrameData())
        {
            return false;
        }

        RegisterBackbuffers();


        return true;
    }

    void VulkanSwapchain::Shutdown()
    {
        m_Device.SetActiveSwapchain(nullptr);

        UnregisterBackbuffers();
        DestroyFrameData();
        DestroySwapchainObjects();
    }

    bool VulkanSwapchain::AcquireNextImage(FrameInfo& outFrame)
    {
        if (m_Swapchain == VK_NULL_HANDLE)
        {
            Resize(m_Description.Width, m_Description.Height);
            if (m_Swapchain == VK_NULL_HANDLE)
            {
                return false;
            }
        }

        VkDevice l_Device = m_Device.GetHandle();
        VulkanFrameData& l_Frame = m_Frames[m_CurrentFrame];

        vkWaitForFences(l_Device, 1, &l_Frame.InFlight, VK_TRUE, UINT64_MAX);

        uint32_t l_ImageIndex = 0;
        VkResult l_Result = vkAcquireNextImageKHR(l_Device, m_Swapchain, UINT64_MAX, l_Frame.ImageAvailable, VK_NULL_HANDLE, &l_ImageIndex);

        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            Resize(m_Extent.width, m_Extent.height);

            return false;
        }

        if (l_Result != VK_SUCCESS && l_Result != VK_SUBOPTIMAL_KHR)
        {

            return false;
        }

        vkResetFences(l_Device, 1, &l_Frame.InFlight);

        m_CurrentImageIndex = l_ImageIndex;

        m_Device.SetActiveSwapchain(this);

        outFrame.BackBuffer = m_BackbufferHandles[l_ImageIndex];
        outFrame.ImageIndex = l_ImageIndex;

        return true;
    }

    void VulkanSwapchain::Present()
    {
        VkSemaphore l_RenderFinished = m_RenderFinishedSemaphores[m_CurrentImageIndex];

        VkPresentInfoKHR l_PresentInfo{};
        l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        l_PresentInfo.waitSemaphoreCount = 1;
        l_PresentInfo.pWaitSemaphores = &l_RenderFinished;
        l_PresentInfo.swapchainCount = 1;
        l_PresentInfo.pSwapchains = &m_Swapchain;
        l_PresentInfo.pImageIndices = &m_CurrentImageIndex;

        VkResult l_Result = vkQueuePresentKHR(m_Device.GetPresentQueue(), &l_PresentInfo);

        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR || l_Result == VK_SUBOPTIMAL_KHR)
        {
            Resize(m_Extent.width, m_Extent.height);
        }
        else if (l_Result != VK_SUCCESS)
        {

        }

        m_CurrentFrame = (m_CurrentFrame + 1) % MaxFramesInFlight;
    }

    void VulkanSwapchain::RegisterBackbuffers()
    {
        VkExtent3D l_Extent{ m_Extent.width, m_Extent.height, 1 };

        m_BackbufferHandles.reserve(m_Images.size());
        for (size_t l_Index = 0; l_Index < m_Images.size(); ++l_Index)
        {
            TextureHandle l_Handle = m_Device.RegisterExternalTexture(m_Images[l_Index], m_ImageViews[l_Index], m_SurfaceFormat.format, l_Extent, VK_IMAGE_ASPECT_COLOR_BIT);
            m_BackbufferHandles.push_back(l_Handle);
        }
    }

    void VulkanSwapchain::UnregisterBackbuffers()
    {
        for (TextureHandle l_Handle : m_BackbufferHandles)
        {
            m_Device.DestroyTexture(l_Handle);
        }

        m_BackbufferHandles.clear();
    }

    VulkanFrameSync VulkanSwapchain::GetCurrentSync() const
    {
        VulkanFrameSync l_Sync;
        l_Sync.Wait = m_Frames[m_CurrentFrame].ImageAvailable;
        l_Sync.Signal = m_RenderFinishedSemaphores[m_CurrentImageIndex];
        l_Sync.Fence = m_Frames[m_CurrentFrame].InFlight;

        return l_Sync;
    }

    void VulkanSwapchain::Resize(uint32_t width, uint32_t height)
    {
        vkDeviceWaitIdle(m_Device.GetHandle());

        m_Description.Width = width;
        m_Description.Height = height;

        UnregisterBackbuffers();
        DestroyFrameData();
        DestroySwapchainObjects();

        if (!CreateSwapchain())
        {
            return;
        }

        CreateImageViews();
        CreateFrameData();
        RegisterBackbuffers();

        m_CurrentFrame = 0;
    }

    bool VulkanSwapchain::CreateSwapchain()
    {
        VulkanSwapchainSupport l_Support = QuerySupport();

        m_SurfaceFormat = ChooseFormat(l_Support.Formats);
        m_PresentMode = ChoosePresentMode(l_Support.PresentModes);
        m_Extent = ChooseExtent(l_Support.Capabilities);
        if (m_Extent.width == 0 || m_Extent.height == 0)
        {
            return false;
        }

        m_FrontendFormat = VulkanFormatToFrontend(m_SurfaceFormat.format);

        uint32_t l_ImageCount = std::max(m_Description.ImageCount, l_Support.Capabilities.minImageCount);
        if (l_Support.Capabilities.maxImageCount > 0)
        {
            l_ImageCount = std::min(l_ImageCount, l_Support.Capabilities.maxImageCount);
        }

        VkSwapchainCreateInfoKHR l_SwapchainCreateInfo{};
        l_SwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        l_SwapchainCreateInfo.surface = m_Device.GetSurface();
        l_SwapchainCreateInfo.minImageCount = l_ImageCount;
        l_SwapchainCreateInfo.imageFormat = m_SurfaceFormat.format;
        l_SwapchainCreateInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
        l_SwapchainCreateInfo.imageExtent = m_Extent;
        l_SwapchainCreateInfo.imageArrayLayers = 1;
        l_SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t l_GraphicsFamily = m_Device.GetGraphicsQueueFamily();
        uint32_t l_PresentFamily = m_Device.GetPresentQueueFamily();
        uint32_t l_QueueFamilies[] = { l_GraphicsFamily, l_PresentFamily };

        if (l_GraphicsFamily != l_PresentFamily)
        {
            l_SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            l_SwapchainCreateInfo.queueFamilyIndexCount = 2;
            l_SwapchainCreateInfo.pQueueFamilyIndices = l_QueueFamilies;
        }
        else
        {
            l_SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        l_SwapchainCreateInfo.preTransform = l_Support.Capabilities.currentTransform;
        l_SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        l_SwapchainCreateInfo.presentMode = m_PresentMode;
        l_SwapchainCreateInfo.clipped = VK_TRUE;
        l_SwapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        VkResult l_Result = vkCreateSwapchainKHR(m_Device.GetHandle(), &l_SwapchainCreateInfo, nullptr, &m_Swapchain);
        if (l_Result != VK_SUCCESS)
        {

            return false;
        }

        uint32_t l_ActualImageCount = 0;
        vkGetSwapchainImagesKHR(m_Device.GetHandle(), m_Swapchain, &l_ActualImageCount, nullptr);
        m_Images.resize(l_ActualImageCount);
        vkGetSwapchainImagesKHR(m_Device.GetHandle(), m_Swapchain, &l_ActualImageCount, m_Images.data());

        return true;
    }

    bool VulkanSwapchain::CreateImageViews()
    {
        m_ImageViews.resize(m_Images.size());

        for (size_t l_Index = 0; l_Index < m_Images.size(); l_Index++)
        {
            VkImageViewCreateInfo l_ViewCreateInfo{};
            l_ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            l_ViewCreateInfo.image = m_Images[l_Index];
            l_ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            l_ViewCreateInfo.format = m_SurfaceFormat.format;
            l_ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            l_ViewCreateInfo.subresourceRange.baseMipLevel = 0;
            l_ViewCreateInfo.subresourceRange.levelCount = 1;
            l_ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            l_ViewCreateInfo.subresourceRange.layerCount = 1;

            VkResult l_Result = vkCreateImageView(m_Device.GetHandle(), &l_ViewCreateInfo, nullptr, &m_ImageViews[l_Index]);
            if (l_Result != VK_SUCCESS)
            {

                return false;
            }
        }

        return true;
    }

    bool VulkanSwapchain::CreateFrameData()
    {
        VkDevice l_Device = m_Device.GetHandle();

        m_Frames.resize(MaxFramesInFlight);
        m_RenderFinishedSemaphores.resize(m_Images.size());

        VkSemaphoreCreateInfo l_SemaphoreCreateInfo{};
        l_SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo l_FenceCreateInfo{};
        l_FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        l_FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t l_Index = 0; l_Index < MaxFramesInFlight; l_Index++)
        {
            VulkanFrameData& l_Frame = m_Frames[l_Index];

            if (vkCreateSemaphore(l_Device, &l_SemaphoreCreateInfo, nullptr, &l_Frame.ImageAvailable) != VK_SUCCESS)
            {

                return false;
            }

            if (vkCreateFence(l_Device, &l_FenceCreateInfo, nullptr, &l_Frame.InFlight) != VK_SUCCESS)
            {

                return false;
            }
        }

        for (size_t l_Index = 0; l_Index < m_Images.size(); l_Index++)
        {
            if (vkCreateSemaphore(l_Device, &l_SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[l_Index]) != VK_SUCCESS)
            {

                return false;
            }
        }

        return true;
    }

    void VulkanSwapchain::DestroyFrameData()
    {
        VkDevice l_Device = m_Device.GetHandle();

        for (VkSemaphore l_Semaphore : m_RenderFinishedSemaphores)
        {
            if (l_Semaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(l_Device, l_Semaphore, nullptr);
            }
        }
        m_RenderFinishedSemaphores.clear();

        for (VulkanFrameData& l_Frame : m_Frames)
        {
            if (l_Frame.InFlight != VK_NULL_HANDLE)
            {
                vkDestroyFence(l_Device, l_Frame.InFlight, nullptr);
                l_Frame.InFlight = VK_NULL_HANDLE;
            }

            if (l_Frame.ImageAvailable != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(l_Device, l_Frame.ImageAvailable, nullptr);
                l_Frame.ImageAvailable = VK_NULL_HANDLE;
            }
        }
        m_Frames.clear();
    }

    void VulkanSwapchain::DestroySwapchainObjects()
    {
        VkDevice l_Device = m_Device.GetHandle();

        for (VkImageView l_View : m_ImageViews)
        {
            if (l_View != VK_NULL_HANDLE)
            {
                vkDestroyImageView(l_Device, l_View, nullptr);
            }
        }

        m_ImageViews.clear();
        m_Images.clear();

        if (m_Swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(l_Device, m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }
    }

    VulkanSwapchainSupport VulkanSwapchain::QuerySupport() const
    {
        VulkanSwapchainSupport l_Support;
        VkPhysicalDevice l_PhysicalDevice = m_Device.GetPhysicalDevice();
        VkSurfaceKHR l_Surface = m_Device.GetSurface();

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(l_PhysicalDevice, l_Surface, &l_Support.Capabilities);

        uint32_t l_FormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(l_PhysicalDevice, l_Surface, &l_FormatCount, nullptr);
        if (l_FormatCount > 0)
        {
            l_Support.Formats.resize(l_FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(l_PhysicalDevice, l_Surface, &l_FormatCount, l_Support.Formats.data());
        }

        uint32_t l_PresentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(l_PhysicalDevice, l_Surface, &l_PresentModeCount, nullptr);
        if (l_PresentModeCount > 0)
        {
            l_Support.PresentModes.resize(l_PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(l_PhysicalDevice, l_Surface, &l_PresentModeCount, l_Support.PresentModes.data());
        }

        return l_Support;
    }

    VkSurfaceFormatKHR VulkanSwapchain::ChooseFormat(const std::vector<VkSurfaceFormatKHR>& available) const
    {
        VkFormat l_Preferred = FrontendFormatToVulkan(m_Description.PreferredFormat);

        for (const VkSurfaceFormatKHR& l_Format : available)
        {
            if (l_Format.format == l_Preferred && l_Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return l_Format;
            }
        }

        return available[0];
    }

    VkPresentModeKHR VulkanSwapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& available) const
    {
        if (m_Description.VSync)
        {
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        for (VkPresentModeKHR l_Mode : available)
        {
            if (l_Mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return l_Mode;
            }
        }

        for (VkPresentModeKHR l_Mode : available)
        {
            if (l_Mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                return l_Mode;
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

        VkExtent2D l_Extent{ m_Description.Width, m_Description.Height };
        l_Extent.width = std::clamp(l_Extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        l_Extent.height = std::clamp(l_Extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return l_Extent;
    }
}