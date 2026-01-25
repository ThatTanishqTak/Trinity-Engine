#include "Engine/Renderer/Vulkan/VulkanDevice.h"

#include "Engine/Utilities/Utilities.h"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace Engine
{
    void VulkanDevice::Initialize(VulkanContext& context)
    {
        m_Context = &context;

        PickPhysicalDevice();
        CreateLogicalDevice();

        TR_CORE_INFO("VulkanDevice initialized.");
    }

    void VulkanDevice::Shutdown()
    {
        if (m_Device)
        {
            vkDeviceWaitIdle(m_Device);
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        m_PhysicalDevice = VK_NULL_HANDLE;
        m_GraphicsQueue = VK_NULL_HANDLE;
        m_PresentQueue = VK_NULL_HANDLE;
        m_QueueFamilies = {};

        m_Context = nullptr;
    }

    void VulkanDevice::PickPhysicalDevice()
    {
        uint32_t it_DeviceCount = 0;
        vkEnumeratePhysicalDevices(m_Context->GetInstance(), &it_DeviceCount, nullptr);

        if (it_DeviceCount == 0)
        {
            TR_CORE_CRITICAL("No Vulkan GPUs found.");

            std::abort();
        }

        std::vector<VkPhysicalDevice> l_Devices(it_DeviceCount);
        vkEnumeratePhysicalDevices(m_Context->GetInstance(), &it_DeviceCount, l_Devices.data());

        for (auto it_Device : l_Devices)
        {
            if (IsDeviceSuitable(it_Device))
            {
                m_PhysicalDevice = it_Device;
                m_QueueFamilies = FindQueueFamilies(it_Device);

                break;
            }
        }

        if (m_PhysicalDevice == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("Failed to find a suitable GPU.");

            std::abort();
        }
    }

    bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device) const
    {
        const auto a_Indices = FindQueueFamilies(device);
        if (!a_Indices.IsComplete())
        {
            return false;
        }

        if (!CheckDeviceExtensionSupport(device))
        {
            return false;
        }

        const auto a_SwapSupport = QuerySwapchainSupport(device);
        if (a_SwapSupport.Formats.empty() || a_SwapSupport.PresentModes.empty())
        {
            return false;
        }

        return true;
    }

    bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
    {
        uint32_t l_ExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &l_ExtensionCount, nullptr);

        std::vector<VkExtensionProperties> it_Available(l_ExtensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &l_ExtensionCount, it_Available.data());

        std::set<std::string> l_Required(m_DeviceExtensions.begin(), m_DeviceExtensions.end());
        for (const auto& a_Extensions : it_Available)
        {
            l_Required.erase(a_Extensions.extensionName);
        }

        return l_Required.empty();
    }

    VulkanDevice::QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) const
    {
        QueueFamilyIndices a_Indices{};

        uint32_t l_QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_QueueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> l_Families(l_QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_QueueFamilyCount, l_Families.data());

        for (uint32_t i = 0; i < l_QueueFamilyCount; ++i)
        {
            if (l_Families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                a_Indices.GraphicsFamily = i;
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Context->GetSurface(), &presentSupport);
            if (presentSupport)
            {
                a_Indices.PresentFamily = i;
            }

            if (a_Indices.IsComplete())
            {
                break;
            }
        }

        return a_Indices;
    }

    VulkanDevice::SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport(VkPhysicalDevice device) const
    {
        SwapchainSupportDetails l_SwapchainSupportDetails{};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Context->GetSurface(), &l_SwapchainSupportDetails.Capabilities);

        uint32_t l_FormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Context->GetSurface(), &l_FormatCount, nullptr);
        if (l_FormatCount)
        {
            l_SwapchainSupportDetails.Formats.resize(l_FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Context->GetSurface(), &l_FormatCount, l_SwapchainSupportDetails.Formats.data());
        }

        uint32_t l_PresentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Context->GetSurface(), &l_PresentModeCount, nullptr);
        if (l_PresentModeCount)
        {
            l_SwapchainSupportDetails.PresentModes.resize(l_PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Context->GetSurface(), &l_PresentModeCount, l_SwapchainSupportDetails.PresentModes.data());
        }

        return l_SwapchainSupportDetails;
    }

    void VulkanDevice::CreateLogicalDevice()
    {
        const auto a_Indices = FindQueueFamilies(m_PhysicalDevice);

        std::set<uint32_t> l_UniqueFamilies = { a_Indices.GraphicsFamily.value(), a_Indices.PresentFamily.value() };

        float l_QueuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> l_DeviceQueueCreateInfoVector;
        l_DeviceQueueCreateInfoVector.reserve(l_UniqueFamilies.size());

        for (uint32_t it_Family : l_UniqueFamilies)
        {
            VkDeviceQueueCreateInfo l_DeviceQueueCreateInfo{};
            l_DeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            l_DeviceQueueCreateInfo.queueFamilyIndex = it_Family;
            l_DeviceQueueCreateInfo.queueCount = 1;
            l_DeviceQueueCreateInfo.pQueuePriorities = &l_QueuePriority;
            l_DeviceQueueCreateInfoVector.push_back(l_DeviceQueueCreateInfo);
        }

        VkPhysicalDeviceFeatures l_PhysicalDeviceFeatures{};

        VkDeviceCreateInfo l_DeviceCreateInfo{};
        l_DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        l_DeviceCreateInfo.queueCreateInfoCount = (uint32_t)l_DeviceQueueCreateInfoVector.size();
        l_DeviceCreateInfo.pQueueCreateInfos = l_DeviceQueueCreateInfoVector.data();
        l_DeviceCreateInfo.pEnabledFeatures = &l_PhysicalDeviceFeatures;

        l_DeviceCreateInfo.enabledExtensionCount = (uint32_t)m_DeviceExtensions.size();
        l_DeviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

        // Device layers are deprecated; instance validation is enough.
        l_DeviceCreateInfo.enabledLayerCount = 0;

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateDevice(m_PhysicalDevice, &l_DeviceCreateInfo, nullptr, &m_Device), "vkCreateDevice");

        vkGetDeviceQueue(m_Device, a_Indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, a_Indices.PresentFamily.value(), 0, &m_PresentQueue);

        m_QueueFamilies = a_Indices;
    }
}