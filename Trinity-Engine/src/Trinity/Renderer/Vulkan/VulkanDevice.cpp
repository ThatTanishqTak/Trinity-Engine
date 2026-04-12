#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Renderer/Vulkan/VulkanDebug.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Platform/Window/Window.h"

#include <SDL3/SDL_vulkan.h>

#include <set>
#include <string>
#include <vector>

namespace Trinity
{
    static const std::vector<const char*> s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
    static const std::vector<const char*> s_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    void VulkanDevice::Initialize(Window& window, bool enableValidation)
    {
        m_ValidationEnabled = enableValidation;

        CreateInstance(enableValidation);
        CreateSurface(window);
        PickPhysicalDevice();
        CreateLogicalDevice(enableValidation);

        TR_CORE_INFO("Vulkan device initialized: {}", m_Properties.deviceName);
    }

    void VulkanDevice::Shutdown()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        if (m_Surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }

        if (m_Instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
        }
    }

    void VulkanDevice::CreateInstance(bool enableValidation)
    {
        if (enableValidation && !CheckValidationLayerSupport())
        {
            TR_CORE_WARN("Validation layers requested but not available. Disabling.");
            m_ValidationEnabled = false;
            enableValidation = false;
        }

        VkApplicationInfo l_AppInfo{};
        l_AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        l_AppInfo.pApplicationName = "Trinity Engine";
        l_AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        l_AppInfo.pEngineName = "Trinity";
        l_AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        l_AppInfo.apiVersion = VK_API_VERSION_1_3;

        uint32_t l_SDLExtensionCount = 0;
        const char* const* l_SDLExtensions = SDL_Vulkan_GetInstanceExtensions(&l_SDLExtensionCount);

        std::vector<const char*> l_Extensions(l_SDLExtensions, l_SDLExtensions + l_SDLExtensionCount);

        if (enableValidation)
        {
            l_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        VkInstanceCreateInfo l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        l_CreateInfo.pApplicationInfo = &l_AppInfo;
        l_CreateInfo.enabledExtensionCount = static_cast<uint32_t>(l_Extensions.size());
        l_CreateInfo.ppEnabledExtensionNames = l_Extensions.data();

#ifdef __APPLE__
        l_CreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        l_Extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        l_CreateInfo.enabledExtensionCount = static_cast<uint32_t>(l_Extensions.size());
        l_CreateInfo.ppEnabledExtensionNames = l_Extensions.data();
#endif

        VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo{};
        if (enableValidation)
        {
            l_CreateInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
            l_CreateInfo.ppEnabledLayerNames = s_ValidationLayers.data();

            VulkanDebug::PopulateCreateInfo(l_DebugCreateInfo);
            l_CreateInfo.pNext = &l_DebugCreateInfo;
        }
        else
        {
            l_CreateInfo.enabledLayerCount = 0;
            l_CreateInfo.pNext = nullptr;
        }

        VulkanUtilities::VKCheck(vkCreateInstance(&l_CreateInfo, nullptr, &m_Instance), "Failed vkCreateInstance");
    }

    void VulkanDevice::CreateSurface(Window& window)
    {
        NativeWindowHandle l_Handle = window.GetNativeHandle();
        if (!SDL_Vulkan_CreateSurface(l_Handle.Window, m_Instance, nullptr, &m_Surface))
        {
            TR_CORE_CRITICAL("Failed to create Vulkan surface: {}", SDL_GetError());
            std::abort();
        }
    }

    void VulkanDevice::PickPhysicalDevice()
    {
        uint32_t l_DeviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &l_DeviceCount, nullptr);

        if (l_DeviceCount == 0)
        {
            TR_CORE_CRITICAL("No GPUs with Vulkan support found.");
            std::abort();
        }

        std::vector<VkPhysicalDevice> l_Devices(l_DeviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &l_DeviceCount, l_Devices.data());

        // Prefer discrete GPU
        for (const auto& l_Device : l_Devices)
        {
            VkPhysicalDeviceProperties l_Props;
            vkGetPhysicalDeviceProperties(l_Device, &l_Props);

            if (IsDeviceSuitable(l_Device) && l_Props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                m_PhysicalDevice = l_Device;
                m_Properties = l_Props;
                m_QueueFamilyIndices = FindQueueFamilies(l_Device);

                return;
            }
        }

        // Fall back to any suitable device
        for (const auto& l_Device : l_Devices)
        {
            if (IsDeviceSuitable(l_Device))
            {
                m_PhysicalDevice = l_Device;
                vkGetPhysicalDeviceProperties(l_Device, &m_Properties);
                m_QueueFamilyIndices = FindQueueFamilies(l_Device);

                return;
            }
        }

        TR_CORE_CRITICAL("No suitable GPU found.");
        std::abort();
    }

    void VulkanDevice::CreateLogicalDevice(bool enableValidation)
    {
        std::set<uint32_t> l_UniqueQueueFamilies = {
            m_QueueFamilyIndices.GraphicsFamily.value(),
            m_QueueFamilyIndices.PresentFamily.value()
        };

        float l_QueuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> l_QueueCreateInfos;

        for (uint32_t l_QueueFamily : l_UniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo l_QueueCreateInfo{};
            l_QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            l_QueueCreateInfo.queueFamilyIndex = l_QueueFamily;
            l_QueueCreateInfo.queueCount = 1;
            l_QueueCreateInfo.pQueuePriorities = &l_QueuePriority;
            l_QueueCreateInfos.push_back(l_QueueCreateInfo);
        }

        VkPhysicalDeviceFeatures l_DeviceFeatures{};
        l_DeviceFeatures.samplerAnisotropy = VK_TRUE;
        l_DeviceFeatures.fillModeNonSolid = VK_TRUE;
        l_DeviceFeatures.wideLines = VK_TRUE;
        l_DeviceFeatures.geometryShader = VK_TRUE;

        VkPhysicalDeviceDynamicRenderingFeatures l_DynamicRenderingFeature{};
        l_DynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        l_DynamicRenderingFeature.dynamicRendering = VK_TRUE;

        VkPhysicalDeviceSynchronization2Features l_Sync2Feature{};
        l_Sync2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        l_Sync2Feature.synchronization2 = VK_TRUE;
        l_Sync2Feature.pNext = &l_DynamicRenderingFeature;

        VkDeviceCreateInfo l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        l_CreateInfo.queueCreateInfoCount = static_cast<uint32_t>(l_QueueCreateInfos.size());
        l_CreateInfo.pQueueCreateInfos = l_QueueCreateInfos.data();
        l_CreateInfo.pEnabledFeatures = &l_DeviceFeatures;
        l_CreateInfo.enabledExtensionCount = static_cast<uint32_t>(s_DeviceExtensions.size());
        l_CreateInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();
        l_CreateInfo.pNext = &l_Sync2Feature;

        if (enableValidation)
        {
            l_CreateInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
            l_CreateInfo.ppEnabledLayerNames = s_ValidationLayers.data();
        }
        else
        {
            l_CreateInfo.enabledLayerCount = 0;
        }

        VulkanUtilities::VKCheck(vkCreateDevice(m_PhysicalDevice, &l_CreateInfo, nullptr, &m_Device), "Failed vkCreateDevice");

        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.PresentFamily.value(), 0, &m_PresentQueue);
    }

    QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) const
    {
        QueueFamilyIndices l_Indices;

        uint32_t l_QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_QueueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> l_QueueFamilies(l_QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_QueueFamilyCount, l_QueueFamilies.data());

        for (uint32_t i = 0; i < l_QueueFamilyCount; i++)
        {
            if (l_QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                l_Indices.GraphicsFamily = i;
            }

            VkBool32 l_PresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &l_PresentSupport);

            if (l_PresentSupport)
            {
                l_Indices.PresentFamily = i;
            }

            if (l_Indices.IsComplete())
            {
                break;
            }
        }

        return l_Indices;
    }

    SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport() const
    {
        SwapchainSupportDetails l_Details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &l_Details.Capabilities);

        uint32_t l_FormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &l_FormatCount, nullptr);
        if (l_FormatCount != 0)
        {
            l_Details.Formats.resize(l_FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &l_FormatCount, l_Details.Formats.data());
        }

        uint32_t l_PresentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &l_PresentModeCount, nullptr);
        if (l_PresentModeCount != 0)
        {
            l_Details.PresentModes.resize(l_PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &l_PresentModeCount, l_Details.PresentModes.data());
        }

        return l_Details;
    }

    bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device) const
    {
        QueueFamilyIndices l_Indices = FindQueueFamilies(device);
        bool l_ExtensionsSupported = CheckDeviceExtensionSupport(device);

        bool l_SwapchainAdequate = false;
        if (l_ExtensionsSupported)
        {
            SwapchainSupportDetails l_SwapchainSupport;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &l_SwapchainSupport.Capabilities);

            uint32_t l_FormatCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &l_FormatCount, nullptr);
            l_SwapchainSupport.Formats.resize(l_FormatCount);
            if (l_FormatCount > 0)
            {
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &l_FormatCount, l_SwapchainSupport.Formats.data());
            }

            uint32_t l_PresentModeCount = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &l_PresentModeCount, nullptr);

            l_SwapchainAdequate = !l_SwapchainSupport.Formats.empty() && l_PresentModeCount > 0;
        }

        VkPhysicalDeviceFeatures l_SupportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &l_SupportedFeatures);

        return l_Indices.IsComplete() && l_ExtensionsSupported && l_SwapchainAdequate && l_SupportedFeatures.samplerAnisotropy;
    }

    bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
    {
        uint32_t l_ExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &l_ExtensionCount, nullptr);

        std::vector<VkExtensionProperties> l_AvailableExtensions(l_ExtensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &l_ExtensionCount, l_AvailableExtensions.data());

        std::set<std::string> l_RequiredExtensions(s_DeviceExtensions.begin(), s_DeviceExtensions.end());

        for (const auto& l_Extension : l_AvailableExtensions)
        {
            l_RequiredExtensions.erase(l_Extension.extensionName);
        }

        return l_RequiredExtensions.empty();
    }

    bool VulkanDevice::CheckValidationLayerSupport() const
    {
        uint32_t l_LayerCount = 0;
        vkEnumerateInstanceLayerProperties(&l_LayerCount, nullptr);

        std::vector<VkLayerProperties> l_AvailableLayers(l_LayerCount);
        vkEnumerateInstanceLayerProperties(&l_LayerCount, l_AvailableLayers.data());

        for (const char* l_LayerName : s_ValidationLayers)
        {
            bool l_Found = false;
            for (const auto& l_LayerProperties : l_AvailableLayers)
            {
                if (strcmp(l_LayerName, l_LayerProperties.layerName) == 0)
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
}