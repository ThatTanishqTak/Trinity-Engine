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

        TR_CORE_INFO("VulkanSwapchain: created ({}x{}, {} images, format {})", m_Extent.width, m_Extent.height, m_Images.size(), static_cast<int>(m_SurfaceFormat.format));
        return true;
    }

    void VulkanSwapchain::Shutdown()
    {
        DestroyFrameData();
        DestroySwapchainObjects();
    }

    bool VulkanSwapchain::AcquireNextImage(FrameInfo& outFrame)
    {
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
            TR_CORE_ERROR("VulkanSwapchain: vkAcquireNextImageKHR failed ({})", static_cast<int>(l_Result));
            return false;
        }

        vkResetFences(l_Device, 1, &l_Frame.InFlight);

        m_CurrentImageIndex = l_ImageIndex;

        outFrame.BackBuffer = TextureHandle{};
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
            TR_CORE_ERROR("VulkanSwapchain: vkQueuePresentKHR failed ({})", static_cast<int>(l_Result));
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % MaxFramesInFlight;
    }

    void VulkanSwapchain::TransitionImageLayout(VkCommandBuffer a_Cmd, VkImage a_Image, VkImageLayout a_From, VkImageLayout a_To)
    {
        VkImageMemoryBarrier2 l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        l_Barrier.oldLayout = a_From;
        l_Barrier.newLayout = a_To;
        l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.image = a_Image;
        l_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        l_Barrier.subresourceRange.baseMipLevel = 0;
        l_Barrier.subresourceRange.levelCount = 1;
        l_Barrier.subresourceRange.baseArrayLayer = 0;
        l_Barrier.subresourceRange.layerCount = 1;

        if (a_From == VK_IMAGE_LAYOUT_UNDEFINED && a_To == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        }
        else if (a_From == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && a_To == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            l_Barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            l_Barrier.dstAccessMask = 0;
        }

        VkDependencyInfo l_DepInfo{};
        l_DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_DepInfo.imageMemoryBarrierCount = 1;
        l_DepInfo.pImageMemoryBarriers = &l_Barrier;

        vkCmdPipelineBarrier2(a_Cmd, &l_DepInfo);
    }

    void VulkanSwapchain::RenderFrame(float red, float green, float blue)
    {
        FrameInfo l_Frame{};
        if (!AcquireNextImage(l_Frame))
        {
            return;
        }

        VulkanFrameData& l_FrameData = m_Frames[m_CurrentFrame];
        VkCommandBuffer l_CommandBuffer = l_FrameData.CommandBuffer;
        VkImage l_Image = m_Images[m_CurrentImageIndex];
        VkImageView l_View = m_ImageViews[m_CurrentImageIndex];

        vkResetCommandBuffer(l_CommandBuffer, 0);

        VkCommandBufferBeginInfo l_CommandBufferBeginInfo{};
        l_CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(l_CommandBuffer, &l_CommandBufferBeginInfo);

        TransitionImageLayout(l_CommandBuffer, l_Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingAttachmentInfo l_ColorAttachment{};
        l_ColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        l_ColorAttachment.imageView = l_View;
        l_ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        l_ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        l_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        l_ColorAttachment.clearValue.color = { { red, green, blue, 1.0f } };

        VkRenderingInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        l_RenderingInfo.renderArea.offset = { 0, 0 };
        l_RenderingInfo.renderArea.extent = m_Extent;
        l_RenderingInfo.layerCount = 1;
        l_RenderingInfo.colorAttachmentCount = 1;
        l_RenderingInfo.pColorAttachments = &l_ColorAttachment;

        vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);
        vkCmdEndRendering(l_CommandBuffer);

        TransitionImageLayout(l_CommandBuffer, l_Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vkEndCommandBuffer(l_CommandBuffer);

        VkSemaphore l_WaitSemaphore = l_FrameData.ImageAvailable;
        VkSemaphore l_SignalSemaphore = m_RenderFinishedSemaphores[m_CurrentImageIndex];

        VkSemaphoreSubmitInfo l_WaitInfo{};
        l_WaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        l_WaitInfo.semaphore = l_WaitSemaphore;
        l_WaitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSemaphoreSubmitInfo l_SignalInfo{};
        l_SignalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        l_SignalInfo.semaphore = l_SignalSemaphore;
        l_SignalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

        VkCommandBufferSubmitInfo l_CommandBufferSubmitInfo{};
        l_CommandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        l_CommandBufferSubmitInfo.commandBuffer = l_CommandBuffer;

        VkSubmitInfo2 l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        l_SubmitInfo.waitSemaphoreInfoCount = 1;
        l_SubmitInfo.pWaitSemaphoreInfos = &l_WaitInfo;
        l_SubmitInfo.commandBufferInfoCount = 1;
        l_SubmitInfo.pCommandBufferInfos = &l_CommandBufferSubmitInfo;
        l_SubmitInfo.signalSemaphoreInfoCount = 1;
        l_SubmitInfo.pSignalSemaphoreInfos = &l_SignalInfo;

        vkQueueSubmit2(m_Device.GetGraphicsQueue(), 1, &l_SubmitInfo, l_FrameData.InFlight);

        Present();
    }

    void VulkanSwapchain::Resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        vkDeviceWaitIdle(m_Device.GetHandle());

        m_Description.Width = width;
        m_Description.Height = height;

        DestroyFrameData();
        DestroySwapchainObjects();
        CreateSwapchain();
        CreateImageViews();
        CreateFrameData();

        m_CurrentFrame = 0;
    }

    bool VulkanSwapchain::CreateSwapchain()
    {
        VulkanSwapchainSupport l_Support = QuerySupport();

        m_SurfaceFormat = ChooseFormat(l_Support.Formats);
        m_PresentMode = ChoosePresentMode(l_Support.PresentModes);
        m_Extent = ChooseExtent(l_Support.Capabilities);
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
            TR_CORE_CRITICAL("VulkanSwapchain: vkCreateSwapchainKHR failed ({})", static_cast<int>(l_Result));
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
                TR_CORE_CRITICAL("VulkanSwapchain: vkCreateImageView failed for image {}", l_Index);
                return false;
            }
        }

        return true;
    }

    bool VulkanSwapchain::CreateFrameData()
    {
        VkDevice l_Device = m_Device.GetHandle();
        VkCommandPool l_CommandPool = m_Device.GetCommands().GetPool();

        m_Frames.resize(MaxFramesInFlight);
        m_RenderFinishedSemaphores.resize(m_Images.size());

        VkCommandBufferAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocateInfo.commandPool = l_CommandPool;
        l_AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocateInfo.commandBufferCount = MaxFramesInFlight;

        std::vector<VkCommandBuffer> l_Buffers(MaxFramesInFlight);
        if (vkAllocateCommandBuffers(l_Device, &l_AllocateInfo, l_Buffers.data()) != VK_SUCCESS)
        {
            TR_CORE_CRITICAL("VulkanSwapchain: vkAllocateCommandBuffers failed");
            return false;
        }

        VkSemaphoreCreateInfo l_SemaphoreCreateInfo{};
        l_SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo l_FenceCreateInfo{};
        l_FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        l_FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t l_Index = 0; l_Index < MaxFramesInFlight; l_Index++)
        {
            VulkanFrameData& l_Frame = m_Frames[l_Index];
            l_Frame.CommandBuffer = l_Buffers[l_Index];

            if (vkCreateSemaphore(l_Device, &l_SemaphoreCreateInfo, nullptr, &l_Frame.ImageAvailable) != VK_SUCCESS)
            {
                TR_CORE_CRITICAL("VulkanSwapchain: vkCreateSemaphore (imageAvailable) failed");
                return false;
            }

            if (vkCreateFence(l_Device, &l_FenceCreateInfo, nullptr, &l_Frame.InFlight) != VK_SUCCESS)
            {
                TR_CORE_CRITICAL("VulkanSwapchain: vkCreateFence failed");
                return false;
            }
        }

        for (size_t l_Index = 0; l_Index < m_Images.size(); l_Index++)
        {
            if (vkCreateSemaphore(l_Device, &l_SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[l_Index]) != VK_SUCCESS)
            {
                TR_CORE_CRITICAL("VulkanSwapchain: vkCreateSemaphore (renderFinished) failed for image {}", l_Index);
                return false;
            }
        }

        return true;
    }

    void VulkanSwapchain::DestroyFrameData()
    {
        VkDevice l_Device = m_Device.GetHandle();
        VkCommandPool l_CommandPool = m_Device.GetCommands().GetPool();

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

            if (l_Frame.CommandBuffer != VK_NULL_HANDLE)
            {
                vkFreeCommandBuffers(l_Device, l_CommandPool, 1, &l_Frame.CommandBuffer);
                l_Frame.CommandBuffer = VK_NULL_HANDLE;
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