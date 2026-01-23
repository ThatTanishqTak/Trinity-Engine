#include "Engine/Renderer/Vulkan/VulkanRenderer.h"
#include "Engine/Platform/Window.h"

#include <GLFW/glfw3.h>

#include <set>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <string>

namespace Engine
{
    static void VKCheck(VkResult result, const char* what)
    {
        if (result == VK_SUCCESS)
        {
            return;
        }

        std::string l_Message = std::string(what) + " failed (VkResult=" + std::to_string((int)result) + ")";
        TR_CORE_CRITICAL("Vulkan failure: {}", l_Message);
        
        throw;
    }

    void VulkanRenderer::Initialize(Window& window)
    {
        Shutdown();

        m_Window = &window;
        m_GLFWWindow = (GLFWwindow*)window.GetNativeWindow();

        try
        {
            CreateInstance();
            SetupDebugMessenger();
            CreateSurface();
            PickPhysicalDevice();
            CreateLogicalDevice();

            CreateSwapchain();
            CreateImageViews();
            CreateRenderPass();
            CreateFramebuffers();
            CreateCommandPool();
            CreateCommandBuffers();
            CreateSyncObjects();

            m_Initialized = true;
            TR_CORE_INFO("Vulkan initialized successfully");
        }
        catch (const std::exception& e)
        {
            TR_CORE_CRITICAL("Vulkan initialization failed: {}", e.what());
            Shutdown();
            
            throw;
        }
    }

    VulkanRenderer::~VulkanRenderer()
    {
        Shutdown();
    }


    void VulkanRenderer::Shutdown()
    {
        // Idempotent shutdown: handle partial initialization and double calls.
        if (m_Device)
        {
            vkDeviceWaitIdle(m_Device);
            CleanupSwapchain();
        }
        else
        {
            // If we never reached device creation, swapchain cleanup would be unsafe.
            m_Framebuffers.clear();
            m_SwapchainImageViews.clear();
            m_SwapchainImages.clear();

            m_Swapchain = VK_NULL_HANDLE;
            m_RenderPass = VK_NULL_HANDLE;
        }

        // Sync objects (only valid if device exists).
        if (m_Device)
        {
            for (size_t i = 0; i < m_ImageAvailableSemaphores.size(); ++i)
            {
                if (m_ImageAvailableSemaphores[i])
                {
                    vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
                }

                if (m_RenderFinishedSemaphores.size() > i && m_RenderFinishedSemaphores[i])
                {
                    vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
                }

                if (m_InFlightFences.size() > i && m_InFlightFences[i])
                {
                    vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
                }
            }

            if (m_CommandPool)
            {
                vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            }
        }

        m_ImageAvailableSemaphores.clear();
        m_RenderFinishedSemaphores.clear();
        m_InFlightFences.clear();
        m_CommandBuffers.clear();
        m_CommandPool = VK_NULL_HANDLE;

        if (m_Device)
        {
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        if (m_Surface && m_Instance)
        {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }

        if (m_EnableValidationLayers && m_DebugMessenger && m_Instance)
        {
            DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
            m_DebugMessenger = VK_NULL_HANDLE;
        }

        if (m_Instance)
        {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
        }

        m_PhysicalDevice = VK_NULL_HANDLE;
        m_GraphicsQueue = VK_NULL_HANDLE;
        m_PresentQueue = VK_NULL_HANDLE;
        m_GLFWWindow = nullptr;
        m_Window = nullptr;
    }

    void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
    {
        m_FramebufferResized = true;

        m_Window->SetWidth(width);
        m_Window->SetHeight(height);
    }

    void VulkanRenderer::BeginFrame()
    {
        if (!m_Initialized)
        {
            return;
        }

        m_FrameInProgress = false;

        VKCheck(vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX), "vkWaitForFences");
        VkResult l_Acquire = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &m_ImageIndex);

        if (l_Acquire == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain();
     
            return;
        }

        if (l_Acquire == VK_SUBOPTIMAL_KHR)
        {
            m_FramebufferResized = true;
        }
        else
        {
            VKCheck(l_Acquire, "vkAcquireNextImageKHR");
        }

        m_FrameInProgress = true;

        VKCheck(vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]), "vkResetFences");
        VKCheck(vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], 0), "vkResetCommandBuffer");

        RecordCommandBuffer(m_CommandBuffers[m_CurrentFrame], m_ImageIndex);

        VkSemaphore l_WaitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
        VkPipelineStageFlags l_WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore l_SignalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.waitSemaphoreCount = 1;
        l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores;
        l_SubmitInfo.pWaitDstStageMask = l_WaitStages;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];
        l_SubmitInfo.signalSemaphoreCount = 1;
        l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores;

        VKCheck(vkQueueSubmit(m_GraphicsQueue, 1, &l_SubmitInfo, m_InFlightFences[m_CurrentFrame]), "vkQueueSubmit");
    }

    void VulkanRenderer::EndFrame()
    {
        if (!m_Initialized)
        {
            return;
        }

        if (!m_FrameInProgress)
        {
            return;
        }

        VkSemaphore l_SignalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };

        VkPresentInfoKHR l_PresentInfo{};
        l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        l_PresentInfo.waitSemaphoreCount = 1;
        l_PresentInfo.pWaitSemaphores = l_SignalSemaphores;
        l_PresentInfo.swapchainCount = 1;
        l_PresentInfo.pSwapchains = &m_Swapchain;
        l_PresentInfo.pImageIndices = &m_ImageIndex;

        VkResult l_Present = vkQueuePresentKHR(m_PresentQueue, &l_PresentInfo);

        if (l_Present == VK_ERROR_OUT_OF_DATE_KHR || l_Present == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
        {
            m_FramebufferResized = false;
            RecreateSwapchain();
        }

        else
        {
            VKCheck(l_Present, "vkQueuePresentKHR");
        }

        m_FrameInProgress = false;

        m_CurrentFrame = (m_CurrentFrame + 1) % s_MaxFramesInFlight;
    }

    void VulkanRenderer::CreateInstance()
    {
        if (m_EnableValidationLayers && !CheckValidationLayerSupport())
        {
            TR_CORE_CRITICAL("Validation l_Layers requested, but not l_Available.");
            throw std::runtime_error("Validation l_Layers not l_Available");
        }

        VkApplicationInfo l_ApplicationInfo{};
        l_ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        l_ApplicationInfo.pApplicationName = "Engine";
        l_ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        l_ApplicationInfo.pEngineName = "Engine";
        l_ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        l_ApplicationInfo.apiVersion = VK_API_VERSION_1_2;

        auto l_Extensions = GetRequiredExtensions();

        VkInstanceCreateInfo l_ImageViewCreateInfo{};
        l_ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        l_ImageViewCreateInfo.pApplicationInfo = &l_ApplicationInfo;
        l_ImageViewCreateInfo.enabledExtensionCount = (uint32_t)l_Extensions.size();
        l_ImageViewCreateInfo.ppEnabledExtensionNames = l_Extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo{};

        if (m_EnableValidationLayers)
        {
            l_ImageViewCreateInfo.enabledLayerCount = (uint32_t)m_ValidationLayers.size();
            l_ImageViewCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();

            l_DebugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            l_DebugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            l_DebugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            l_DebugCreateInfo.pfnUserCallback = DebugCallback;

            l_ImageViewCreateInfo.pNext = &l_DebugCreateInfo;
        }

        else
        {
            l_ImageViewCreateInfo.enabledLayerCount = 0;
            l_ImageViewCreateInfo.pNext = nullptr;
        }

        VKCheck(vkCreateInstance(&l_ImageViewCreateInfo, nullptr, &m_Instance), "vkCreateInstance");
    }

    void VulkanRenderer::SetupDebugMessenger()
    {
        if (!m_EnableValidationLayers)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT l_ImageViewCreateInfo{};
        l_ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        l_ImageViewCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        l_ImageViewCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        l_ImageViewCreateInfo.pfnUserCallback = DebugCallback;

        VKCheck(CreateDebugUtilsMessengerEXT(m_Instance, &l_ImageViewCreateInfo, nullptr, &m_DebugMessenger), "CreateDebugUtilsMessengerEXT");
    }

    void VulkanRenderer::CreateSurface()
    {
        VKCheck(glfwCreateWindowSurface(m_Instance, m_GLFWWindow, nullptr, &m_Surface), "glfwCreateWindowSurface");
    }

    VulkanRenderer::QueueFamilyIndices VulkanRenderer::FindQueueFamilies(VkPhysicalDevice it_Device) const
    {
        QueueFamilyIndices l_QueueFamily;

        uint32_t l_QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(it_Device, &l_QueueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> l_Families(l_QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(it_Device, &l_QueueFamilyCount, l_Families.data());

        int i = 0;
        for (const auto& it_Family : l_Families)
        {
            if (it_Family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                l_QueueFamily.GraphicsFamily = i;
            }

            VkBool32 l_PresentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(it_Device, i, m_Surface, &l_PresentSupport);
            if (l_PresentSupport)
            {
                l_QueueFamily.PresentFamily = i;
            }

            if (l_QueueFamily.IsComplete())
            {
                break;
            }

            ++i;
        }

        return l_QueueFamily;
    }

    VulkanRenderer::SwapchainSupportDetails VulkanRenderer::QuerySwapchainSupport(VkPhysicalDevice device) const
    {
        SwapchainSupportDetails l_Details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &l_Details.Capabilities);

        uint32_t l_FormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &l_FormatCount, nullptr);
        if (l_FormatCount)
        {
            l_Details.Formats.resize(l_FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &l_FormatCount, l_Details.Formats.data());
        }

        uint32_t l_PresentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &l_PresentModeCount, nullptr);
        if (l_PresentModeCount)
        {
            l_Details.PresentModes.resize(l_PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &l_PresentModeCount, l_Details.PresentModes.data());
        }

        return l_Details;
    }

    void VulkanRenderer::PickPhysicalDevice()
    {
        uint32_t l_DeviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &l_DeviceCount, nullptr);

        if (l_DeviceCount == 0)
        {
            TR_CORE_CRITICAL("No Vulkan GPUs l_Found.");
            throw std::runtime_error("No Vulkan GPU");
        }

        std::vector<VkPhysicalDevice> l_Devices(l_DeviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &l_DeviceCount, l_Devices.data());

        for (auto it_Device : l_Devices)
        {
            auto l_QueueFamily = FindQueueFamilies(it_Device);
            if (!l_QueueFamily.IsComplete())
            {
                continue;
            }

            // Extension support
            uint32_t l_ExtensionCount = 0;
            vkEnumerateDeviceExtensionProperties(it_Device, nullptr, &l_ExtensionCount, nullptr);
            std::vector<VkExtensionProperties> l_Available(l_ExtensionCount);
            vkEnumerateDeviceExtensionProperties(it_Device, nullptr, &l_ExtensionCount, l_Available.data());

            std::set<std::string> l_Required(m_DeviceExtensions.begin(), m_DeviceExtensions.end());
            for (const auto& it_Extension : l_Available)
            {
                l_Required.erase(it_Extension.extensionName);
            }

            if (!l_Required.empty())
            {
                continue;
            }

            auto a_SwapSupport = QuerySwapchainSupport(it_Device);
            if (a_SwapSupport.Formats.empty() || a_SwapSupport.PresentModes.empty())
            {
                continue;
            }

            m_PhysicalDevice = it_Device;
            break;
        }

        if (m_PhysicalDevice == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("Failed to find a suitable GPU.");
            throw std::runtime_error("No suitable GPU");
        }
    }

    void VulkanRenderer::CreateLogicalDevice()
    {
        QueueFamilyIndices l_QueueFamily = FindQueueFamilies(m_PhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> l_DeviceQueueCreateInfo;
        std::set<uint32_t> l_UniqueFamilies =
        {
            l_QueueFamily.GraphicsFamily.value(),
            l_QueueFamily.PresentFamily.value()
        };

        float queuePriority = 1.0f;
        for (uint32_t it_Family : l_UniqueFamilies)
        {
            VkDeviceQueueCreateInfo l_DeviceQueueInfo{};
            l_DeviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            l_DeviceQueueInfo.queueFamilyIndex = it_Family;
            l_DeviceQueueInfo.queueCount = 1;
            l_DeviceQueueInfo.pQueuePriorities = &queuePriority;
            l_DeviceQueueCreateInfo.push_back(l_DeviceQueueInfo);
        }

        VkPhysicalDeviceFeatures l_PhysicalDeviceFeature{};

        VkDeviceCreateInfo l_ImageViewCreateInfo{};
        l_ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        l_ImageViewCreateInfo.queueCreateInfoCount = (uint32_t)l_DeviceQueueCreateInfo.size();
        l_ImageViewCreateInfo.pQueueCreateInfos = l_DeviceQueueCreateInfo.data();
        l_ImageViewCreateInfo.pEnabledFeatures = &l_PhysicalDeviceFeature;

        l_ImageViewCreateInfo.enabledExtensionCount = (uint32_t)m_DeviceExtensions.size();
        l_ImageViewCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

        if (m_EnableValidationLayers)
        {
            l_ImageViewCreateInfo.enabledLayerCount = (uint32_t)m_ValidationLayers.size();
            l_ImageViewCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();
        }

        else
        {
            l_ImageViewCreateInfo.enabledLayerCount = 0;
        }

        VKCheck(vkCreateDevice(m_PhysicalDevice, &l_ImageViewCreateInfo, nullptr, &m_Device), "vkCreateDevice");

        vkGetDeviceQueue(m_Device, l_QueueFamily.GraphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, l_QueueFamily.PresentFamily.value(), 0, &m_PresentQueue);
    }

    VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
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

    VkPresentModeKHR VulkanRenderer::ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) const
    {
        for (auto it_Mode : modes)
        {
            if (it_Mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return it_Mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }

        int l_Width = 0, l_Height = 0;
        glfwGetFramebufferSize(m_GLFWWindow, &l_Width, &l_Height);

        VkExtent2D l_SwapchainExtent{};
        l_SwapchainExtent.width = (uint32_t)std::clamp(l_Width, (int)capabilities.minImageExtent.width, (int)capabilities.maxImageExtent.width);
        l_SwapchainExtent.height = (uint32_t)std::clamp(l_Height, (int)capabilities.minImageExtent.height, (int)capabilities.maxImageExtent.height);

        return l_SwapchainExtent;
    }

    void VulkanRenderer::CreateSwapchain()
    {
        auto a_Support = QuerySwapchainSupport(m_PhysicalDevice);
        auto a_SwapchainImageFormat = ChooseSwapSurfaceFormat(a_Support.Formats);
        auto a_PresentMode = ChoosePresentMode(a_Support.PresentModes);
        auto a_SwapchainExtent = ChooseSwapExtent(a_Support.Capabilities);

        uint32_t l_ImageCount = a_Support.Capabilities.minImageCount + 1;
        if (a_Support.Capabilities.maxImageCount > 0 && l_ImageCount > a_Support.Capabilities.maxImageCount)
        {
            l_ImageCount = a_Support.Capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR l_ImageViewCreateInfo{};
        l_ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        l_ImageViewCreateInfo.surface = m_Surface;
        l_ImageViewCreateInfo.minImageCount = l_ImageCount;
        l_ImageViewCreateInfo.imageFormat = a_SwapchainImageFormat.format;
        l_ImageViewCreateInfo.imageColorSpace = a_SwapchainImageFormat.colorSpace;
        l_ImageViewCreateInfo.imageExtent = a_SwapchainExtent;
        l_ImageViewCreateInfo.imageArrayLayers = 1;
        l_ImageViewCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices l_QueueFamily = FindQueueFamilies(m_PhysicalDevice);
        uint32_t l_QueueFamilyIndices[] = { l_QueueFamily.GraphicsFamily.value(), l_QueueFamily.PresentFamily.value() };

        if (l_QueueFamily.GraphicsFamily != l_QueueFamily.PresentFamily)
        {
            l_ImageViewCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            l_ImageViewCreateInfo.queueFamilyIndexCount = 2;
            l_ImageViewCreateInfo.pQueueFamilyIndices = l_QueueFamilyIndices;
        }

        else
        {
            l_ImageViewCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            l_ImageViewCreateInfo.queueFamilyIndexCount = 0;
            l_ImageViewCreateInfo.pQueueFamilyIndices = nullptr;
        }

        l_ImageViewCreateInfo.preTransform = a_Support.Capabilities.currentTransform;
        l_ImageViewCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        l_ImageViewCreateInfo.presentMode = a_PresentMode;
        l_ImageViewCreateInfo.clipped = VK_TRUE;
        l_ImageViewCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        VKCheck(vkCreateSwapchainKHR(m_Device, &l_ImageViewCreateInfo, nullptr, &m_Swapchain), "vkCreateSwapchainKHR");

        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &l_ImageCount, nullptr);
        m_SwapchainImages.resize(l_ImageCount);
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &l_ImageCount, m_SwapchainImages.data());

        m_SwapchainImageFormat = a_SwapchainImageFormat.format;
        m_SwapchainExtent = a_SwapchainExtent;
    }

    void VulkanRenderer::CreateImageViews()
    {
        m_SwapchainImageViews.resize(m_SwapchainImages.size());

        for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
        {
            VkImageViewCreateInfo l_ImageViewCreateInfo{};
            l_ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            l_ImageViewCreateInfo.image = m_SwapchainImages[i];
            l_ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            l_ImageViewCreateInfo.format = m_SwapchainImageFormat;
            l_ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            l_ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            l_ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            l_ImageViewCreateInfo.subresourceRange.levelCount = 1;
            l_ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            l_ImageViewCreateInfo.subresourceRange.layerCount = 1;

            VKCheck(vkCreateImageView(m_Device, &l_ImageViewCreateInfo, nullptr, &m_SwapchainImageViews[i]), "vkCreateImageView");
        }
    }

    void VulkanRenderer::CreateRenderPass()
    {
        VkAttachmentDescription l_ColorAttachmentDescription{};
        l_ColorAttachmentDescription.format = m_SwapchainImageFormat;
        l_ColorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        l_ColorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        l_ColorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        l_ColorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        l_ColorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        l_ColorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        l_ColorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference l_ColorAttachmentReference{};
        l_ColorAttachmentReference.attachment = 0;
        l_ColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription l_SubpassDescription{};
        l_SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        l_SubpassDescription.colorAttachmentCount = 1;
        l_SubpassDescription.pColorAttachments = &l_ColorAttachmentReference;

        VkSubpassDependency l_SubpassDependency{};
        l_SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        l_SubpassDependency.dstSubpass = 0;
        l_SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        l_SubpassDependency.srcAccessMask = 0;
        l_SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        l_SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo l_RenderPassCreateInfo{};
        l_RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        l_RenderPassCreateInfo.attachmentCount = 1;
        l_RenderPassCreateInfo.pAttachments = &l_ColorAttachmentDescription;
        l_RenderPassCreateInfo.subpassCount = 1;
        l_RenderPassCreateInfo.pSubpasses = &l_SubpassDescription;
        l_RenderPassCreateInfo.dependencyCount = 1;
        l_RenderPassCreateInfo.pDependencies = &l_SubpassDependency;

        VKCheck(vkCreateRenderPass(m_Device, &l_RenderPassCreateInfo, nullptr, &m_RenderPass), "vkCreateRenderPass");
    }

    void VulkanRenderer::CreateFramebuffers()
    {
        m_Framebuffers.resize(m_SwapchainImageViews.size());

        for (size_t i = 0; i < m_SwapchainImageViews.size(); ++i)
        {
            VkImageView l_ImageViewAttachments[] = { m_SwapchainImageViews[i] };

            VkFramebufferCreateInfo l_FrameBufferCreateInfo{};
            l_FrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            l_FrameBufferCreateInfo.renderPass = m_RenderPass;
            l_FrameBufferCreateInfo.attachmentCount = 1;
            l_FrameBufferCreateInfo.pAttachments = l_ImageViewAttachments;
            l_FrameBufferCreateInfo.width = m_SwapchainExtent.width;
            l_FrameBufferCreateInfo.height = m_SwapchainExtent.height;
            l_FrameBufferCreateInfo.layers = 1;

            VKCheck(vkCreateFramebuffer(m_Device, &l_FrameBufferCreateInfo, nullptr, &m_Framebuffers[i]), "vkCreateFramebuffer");
        }
    }

    void VulkanRenderer::CreateCommandPool()
    {
        QueueFamilyIndices l_QueueFamily = FindQueueFamilies(m_PhysicalDevice);

        VkCommandPoolCreateInfo l_CommandPoolCreateInfo{};
        l_CommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        l_CommandPoolCreateInfo.queueFamilyIndex = l_QueueFamily.GraphicsFamily.value();

        VKCheck(vkCreateCommandPool(m_Device, &l_CommandPoolCreateInfo, nullptr, &m_CommandPool), "vkCreateCommandPool");
    }

    void VulkanRenderer::CreateCommandBuffers()
    {
        m_CommandBuffers.resize(s_MaxFramesInFlight);

        VkCommandBufferAllocateInfo l_CommandBufferAllocateInfo{};
        l_CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_CommandBufferAllocateInfo.commandPool = m_CommandPool;
        l_CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_CommandBufferAllocateInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

        VKCheck(vkAllocateCommandBuffers(m_Device, &l_CommandBufferAllocateInfo, m_CommandBuffers.data()), "vkAllocateCommandBuffers");
    }

    void VulkanRenderer::CreateSyncObjects()
    {
        m_ImageAvailableSemaphores.resize(s_MaxFramesInFlight);
        m_RenderFinishedSemaphores.resize(s_MaxFramesInFlight);
        m_InFlightFences.resize(s_MaxFramesInFlight);

        VkSemaphoreCreateInfo l_SemaphoreCreateInfo{};
        l_SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo l_FenceInfo{};
        l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        l_FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < s_MaxFramesInFlight; ++i)
        {
            VKCheck(vkCreateSemaphore(m_Device, &l_SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i]), "vkCreateSemaphore (image)");
            VKCheck(vkCreateSemaphore(m_Device, &l_SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]), "vkCreateSemaphore (render)");
            VKCheck(vkCreateFence(m_Device, &l_FenceInfo, nullptr, &m_InFlightFences[i]), "vkCreateFence");
        }
    }

    void VulkanRenderer::RecordCommandBuffer(VkCommandBuffer command, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo l_CommandBufferBeginInfo{};
        l_CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VKCheck(vkBeginCommandBuffer(command, &l_CommandBufferBeginInfo), "vkBeginCommandBuffer");

        VkClearValue l_ClearColor{};
        l_ClearColor.color.float32[0] = 0.05f;
        l_ClearColor.color.float32[1] = 0.05f;
        l_ClearColor.color.float32[2] = 0.08f;
        l_ClearColor.color.float32[3] = 1.0f;

        VkRenderPassBeginInfo l_RenderPassBeginInfo{};
        l_RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        l_RenderPassBeginInfo.renderPass = m_RenderPass;
        l_RenderPassBeginInfo.framebuffer = m_Framebuffers[imageIndex];
        l_RenderPassBeginInfo.renderArea.offset = { 0, 0 };
        l_RenderPassBeginInfo.renderArea.extent = m_SwapchainExtent;
        l_RenderPassBeginInfo.clearValueCount = 1;
        l_RenderPassBeginInfo.pClearValues = &l_ClearColor;

        vkCmdBeginRenderPass(command, &l_RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(command);

        VKCheck(vkEndCommandBuffer(command), "vkEndCommandBuffer");
    }

    void VulkanRenderer::CleanupSwapchain()
    {
        for (auto it_Framebuffer : m_Framebuffers)
        {
            vkDestroyFramebuffer(m_Device, it_Framebuffer, nullptr);
        }
        m_Framebuffers.clear();

        if (m_RenderPass)
        {
            vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
        }
        m_RenderPass = VK_NULL_HANDLE;

        for (auto it_View : m_SwapchainImageViews)
        {
            vkDestroyImageView(m_Device, it_View, nullptr);
        }
        m_SwapchainImageViews.clear();

        if (m_Swapchain)
        {
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
        }
        m_Swapchain = VK_NULL_HANDLE;
    }

    void VulkanRenderer::RecreateSwapchain()
    {
        int l_Width = 0;
        int l_Height = 0;
        glfwGetFramebufferSize(m_GLFWWindow, &l_Width, &l_Height);

        if (l_Width == 0 || l_Height == 0)
        {
            return;
        }

        vkDeviceWaitIdle(m_Device);

        CleanupSwapchain();

        CreateSwapchain();
        CreateImageViews();
        CreateRenderPass();
        CreateFramebuffers();

        TR_CORE_INFO("Swapchain recreated ({}x{})", m_SwapchainExtent.width, m_SwapchainExtent.height);
    }

    bool VulkanRenderer::CheckValidationLayerSupport() const
    {
        uint32_t l_LayerCount = 0;
        vkEnumerateInstanceLayerProperties(&l_LayerCount, nullptr);

        std::vector<VkLayerProperties> l_Layers(l_LayerCount);
        vkEnumerateInstanceLayerProperties(&l_LayerCount, l_Layers.data());

        for (const char* it_Name : m_ValidationLayers)
        {
            bool l_Found = false;
            for (const auto& it_Layer : l_Layers)
            {
                if (strcmp(it_Name, it_Layer.layerName) == 0)
                {
                    l_Found = true;
                    break;
                }
            }
            if (!l_Found)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> VulkanRenderer::GetRequiredExtensions() const
    {
        uint32_t l_GLFWCount = 0;
        const char** l_GLFWExtensions = glfwGetRequiredInstanceExtensions(&l_GLFWCount);

        std::vector<const char*> l_Extensions(l_GLFWExtensions, l_GLFWExtensions + l_GLFWCount);

        if (m_EnableValidationLayers)
        {
            l_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return l_Extensions;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT, 
        const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
    {
        if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            TR_CORE_ERROR("Vulkan validation: {}", data->pMessage);
        }

        else
        {
            TR_CORE_WARN("Vulkan validation: {}", data->pMessage);
        }

        return VK_FALSE;
    }

    VkResult VulkanRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, 
        VkDebugUtilsMessengerEXT* pMessenger)
    {
        auto a_Function = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (a_Function)
        {
            return a_Function(instance, pCreateInfo, pAllocator, pMessenger);
        }

        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
    {
        auto a_Function = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (a_Function)
        {
            a_Function(instance, messenger, pAllocator);
        }
    }
}