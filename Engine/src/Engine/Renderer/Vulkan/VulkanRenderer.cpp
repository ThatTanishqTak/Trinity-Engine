#include "Engine/Renderer/Vulkan/VulkanRenderer.h"
#include "Engine/Platform/Window.h"

#include <GLFW/glfw3.h>

#include <set>
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace Engine
{
    static void VKCheck(VkResult result, const char* what)
    {
        if (result != VK_SUCCESS)
        {
            TR_CORE_CRITICAL("Vulkan failure: {} (VkResult={})", what, (int)result);
            throw std::runtime_error(what);
        }
    }

    void VulkanRenderer::Initialize(Window& window)
    {
        m_Window = &window;
        m_GLFWWindow = (GLFWwindow*)window.GetNativeWindow();

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

        TR_CORE_INFO("Vulkan initialized successfully.");
    }

    void VulkanRenderer::Shutdown()
    {
        vkDeviceWaitIdle(m_Device);

        CleanupSwapchain();

        for (int i = 0; i < s_MaxFramesInFlight; ++i)
        {
            if (m_ImageAvailableSemaphores[i])
            {
                vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
            }

            if (m_RenderFinishedSemaphores[i])
            {
                vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
            }

            if (m_InFlightFences[i])
            {
                vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
            }
        }

        if (m_CommandPool)
        {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
        }
        
        if (m_Device)
        {
            vkDestroyDevice(m_Device, nullptr);
        }

        if (m_Surface)
        {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        }

        if (m_EnableValidationLayers && m_DebugMessenger)
        {
            DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
        }

        if (m_Instance)
        {
            vkDestroyInstance(m_Instance, nullptr);
        }

        m_Device = VK_NULL_HANDLE;
        m_Instance = VK_NULL_HANDLE;
    }

    void VulkanRenderer::OnResize(uint32_t, uint32_t)
    {
        m_FramebufferResized = true;
    }

    void VulkanRenderer::BeginFrame()
    {
        VKCheck(vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX), "vkWaitForFences");
        VkResult acquire = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &m_ImageIndex);

        if (acquire == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain();
            return;
        }

        VKCheck(acquire, "vkAcquireNextImageKHR");

        VKCheck(vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]), "vkResetFences");
        VKCheck(vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], 0), "vkResetCommandBuffer");

        RecordCommandBuffer(m_CommandBuffers[m_CurrentFrame], m_ImageIndex);

        VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VKCheck(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]), "vkQueueSubmit");
    }

    void VulkanRenderer::EndFrame()
    {
        VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &m_ImageIndex;

        VkResult present = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

        if (present == VK_ERROR_OUT_OF_DATE_KHR || present == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
        {
            m_FramebufferResized = false;
            RecreateSwapchain();
        }

        else
        {
            VKCheck(present, "vkQueuePresentKHR");
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % s_MaxFramesInFlight;
    }

    void VulkanRenderer::CreateInstance()
    {
        if (m_EnableValidationLayers && !CheckValidationLayerSupport())
        {
            TR_CORE_CRITICAL("Validation layers requested, but not available.");
            throw std::runtime_error("Validation layers not available");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        auto extensions = GetRequiredExtensions();

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = (uint32_t)extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        if (m_EnableValidationLayers)
        {
            createInfo.enabledLayerCount = (uint32_t)m_ValidationLayers.size();
            createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = DebugCallback;

            createInfo.pNext = &debugCreateInfo;
        }

        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        VKCheck(vkCreateInstance(&createInfo, nullptr, &m_Instance), "vkCreateInstance");
    }

    void VulkanRenderer::SetupDebugMessenger()
    {
        if (!m_EnableValidationLayers)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;

        VKCheck(CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger), "CreateDebugUtilsMessengerEXT");
    }

    void VulkanRenderer::CreateSurface()
    {
        VKCheck(glfwCreateWindowSurface(m_Instance, m_GLFWWindow, nullptr, &m_Surface), "glfwCreateWindowSurface");
    }

    VulkanRenderer::QueueFamilyIndices VulkanRenderer::FindQueueFamilies(VkPhysicalDevice device) const
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, families.data());

        int i = 0;
        for (const auto& family : families)
        {
            if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.GraphicsFamily = i;
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
            if (presentSupport)
            {
                indices.PresentFamily = i;
            }

            if (indices.IsComplete())
            {
                break;
            }

            ++i;
        }

        return indices;
    }

    VulkanRenderer::SwapchainSupportDetails VulkanRenderer::QuerySwapchainSupport(VkPhysicalDevice device) const
    {
        SwapchainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.Capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
        if (formatCount)
        {
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.Formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
        if (presentModeCount)
        {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.PresentModes.data());
        }

        return details;
    }

    void VulkanRenderer::PickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            TR_CORE_CRITICAL("No Vulkan GPUs found.");
            throw std::runtime_error("No Vulkan GPU");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        for (auto device : devices)
        {
            auto indices = FindQueueFamilies(device);
            if (!indices.IsComplete())
            {
                continue;
            }

            // Extension support
            uint32_t extCount = 0;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
            std::vector<VkExtensionProperties> available(extCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, available.data());

            std::set<std::string> required(m_DeviceExtensions.begin(), m_DeviceExtensions.end());
            for (const auto& ext : available)
            {
                required.erase(ext.extensionName);
            }

            if (!required.empty())
            {
                continue;
            }

            auto swapSupport = QuerySwapchainSupport(device);
            if (swapSupport.Formats.empty() || swapSupport.PresentModes.empty())
            {
                continue;
            }

            m_PhysicalDevice = device;
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
        QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueFamilies =
        {
            indices.GraphicsFamily.value(),
            indices.PresentFamily.value()
        };

        float queuePriority = 1.0f;
        for (uint32_t family : uniqueFamilies)
        {
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = family;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueInfo);
        }

        VkPhysicalDeviceFeatures features{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &features;

        createInfo.enabledExtensionCount = (uint32_t)m_DeviceExtensions.size();
        createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

        if (m_EnableValidationLayers)
        {
            createInfo.enabledLayerCount = (uint32_t)m_ValidationLayers.size();
            createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
        }

        else
        {
            createInfo.enabledLayerCount = 0;
        }

        VKCheck(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device), "vkCreateDevice");

        vkGetDeviceQueue(m_Device, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, indices.PresentFamily.value(), 0, &m_PresentQueue);
    }

    VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
    {
        for (const auto& f : formats)
        {
            if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return f;
            }
        }

        return formats[0];
    }

    VkPresentModeKHR VulkanRenderer::ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes) const
    {
        for (auto m : modes)
        {
            if (m == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return m;
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

        int w = 0, h = 0;
        glfwGetFramebufferSize(m_GLFWWindow, &w, &h);

        VkExtent2D extent{};
        extent.width = (uint32_t)std::clamp(w, (int)capabilities.minImageExtent.width, (int)capabilities.maxImageExtent.width);
        extent.height = (uint32_t)std::clamp(h, (int)capabilities.minImageExtent.height, (int)capabilities.maxImageExtent.height);

        return extent;
    }

    void VulkanRenderer::CreateSwapchain()
    {
        auto support = QuerySwapchainSupport(m_PhysicalDevice);

        auto surfaceFormat = ChooseSwapSurfaceFormat(support.Formats);
        auto presentMode = ChoosePresentMode(support.PresentModes);
        auto extent = ChooseSwapExtent(support.Capabilities);

        uint32_t imageCount = support.Capabilities.minImageCount + 1;
        if (support.Capabilities.maxImageCount > 0 && imageCount > support.Capabilities.maxImageCount)
        {
            imageCount = support.Capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_Surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
        uint32_t queueFamilyIndices[] = { indices.GraphicsFamily.value(), indices.PresentFamily.value() };

        if (indices.GraphicsFamily != indices.PresentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }

        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = support.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VKCheck(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain), "vkCreateSwapchainKHR");

        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
        m_SwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());

        m_SwapchainImageFormat = surfaceFormat.format;
        m_SwapchainExtent = extent;
    }

    void VulkanRenderer::CreateImageViews()
    {
        m_SwapchainImageViews.resize(m_SwapchainImages.size());

        for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_SwapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_SwapchainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VKCheck(vkCreateImageView(m_Device, &createInfo, nullptr, &m_SwapchainImageViews[i]), "vkCreateImageView");
        }
    }

    void VulkanRenderer::CreateRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_SwapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VKCheck(vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass), "vkCreateRenderPass");
    }

    void VulkanRenderer::CreateFramebuffers()
    {
        m_Framebuffers.resize(m_SwapchainImageViews.size());

        for (size_t i = 0; i < m_SwapchainImageViews.size(); ++i)
        {
            VkImageView attachments[] = { m_SwapchainImageViews[i] };

            VkFramebufferCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            info.renderPass = m_RenderPass;
            info.attachmentCount = 1;
            info.pAttachments = attachments;
            info.width = m_SwapchainExtent.width;
            info.height = m_SwapchainExtent.height;
            info.layers = 1;

            VKCheck(vkCreateFramebuffer(m_Device, &info, nullptr, &m_Framebuffers[i]), "vkCreateFramebuffer");
        }
    }

    void VulkanRenderer::CreateCommandPool()
    {
        QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = indices.GraphicsFamily.value();

        VKCheck(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool), "vkCreateCommandPool");
    }

    void VulkanRenderer::CreateCommandBuffers()
    {
        m_CommandBuffers.resize(s_MaxFramesInFlight);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

        VKCheck(vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()), "vkAllocateCommandBuffers");
    }

    void VulkanRenderer::CreateSyncObjects()
    {
        m_ImageAvailableSemaphores.resize(s_MaxFramesInFlight);
        m_RenderFinishedSemaphores.resize(s_MaxFramesInFlight);
        m_InFlightFences.resize(s_MaxFramesInFlight);

        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < s_MaxFramesInFlight; ++i)
        {
            VKCheck(vkCreateSemaphore(m_Device, &semInfo, nullptr, &m_ImageAvailableSemaphores[i]), "vkCreateSemaphore (image)");
            VKCheck(vkCreateSemaphore(m_Device, &semInfo, nullptr, &m_RenderFinishedSemaphores[i]), "vkCreateSemaphore (render)");
            VKCheck(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]), "vkCreateFence");
        }
    }

    void VulkanRenderer::RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VKCheck(vkBeginCommandBuffer(cmd, &beginInfo), "vkBeginCommandBuffer");

        VkClearValue clearColor{};
        clearColor.color.float32[0] = 0.05f;
        clearColor.color.float32[1] = 0.05f;
        clearColor.color.float32[2] = 0.08f;
        clearColor.color.float32[3] = 1.0f;

        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = m_RenderPass;
        rpInfo.framebuffer = m_Framebuffers[imageIndex];
        rpInfo.renderArea.offset = { 0, 0 };
        rpInfo.renderArea.extent = m_SwapchainExtent;
        rpInfo.clearValueCount = 1;
        rpInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(cmd);

        VKCheck(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");
    }

    void VulkanRenderer::CleanupSwapchain()
    {
        for (auto fb : m_Framebuffers)
        {
            vkDestroyFramebuffer(m_Device, fb, nullptr);
        }
        m_Framebuffers.clear();

        if (m_RenderPass)
        {
            vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
        }
        m_RenderPass = VK_NULL_HANDLE;

        for (auto view : m_SwapchainImageViews)
        {
            vkDestroyImageView(m_Device, view, nullptr);
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
        int w = 0, h = 0;
        glfwGetFramebufferSize(m_GLFWWindow, &w, &h);

        if (w == 0 || h == 0)
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
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        for (const char* name : m_ValidationLayers)
        {
            bool found = false;
            for (const auto& layer : layers)
            {
                if (strcmp(name, layer.layerName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> VulkanRenderer::GetRequiredExtensions() const
    {
        uint32_t glfwCount = 0;
        const char** glfwExt = glfwGetRequiredInstanceExtensions(&glfwCount);

        std::vector<const char*> extensions(glfwExt, glfwExt + glfwCount);

        if (m_EnableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
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
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func)
        {
            return func(instance, pCreateInfo, pAllocator, pMessenger);
        }

        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func)
        {
            func(instance, messenger, pAllocator);
        }
    }
}