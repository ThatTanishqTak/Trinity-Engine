#include <Trinity/Renderer/Backends/Vulkan/VulkanPhysicalDevice.h>

#include <cstring>
#include <set>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    static const std::vector<const char*> s_RequiredDeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    bool VulkanPhysicalDevice::Select(VkInstance instance, VkSurfaceKHR surface, const VulkanFeatures& required)
    {
        m_RequiredExtensions = s_RequiredDeviceExtensions;

        uint32_t l_DeviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &l_DeviceCount, nullptr);

        if (l_DeviceCount == 0)
        {
            TR_CORE_CRITICAL("VulkanPhysicalDevice: no Vulkan-capable GPUs found");
            return false;
        }

        std::vector<VkPhysicalDevice> l_Devices(l_DeviceCount);
        vkEnumeratePhysicalDevices(instance, &l_DeviceCount, l_Devices.data());

        VkPhysicalDevice l_Best = VK_NULL_HANDLE;
        uint32_t l_BestScore = 0;

        for (VkPhysicalDevice l_Candidate : l_Devices)
        {
            if (!IsSuitable(l_Candidate, surface, required))
            {
                continue;
            }

            uint32_t l_Score = ScoreDevice(l_Candidate);
            if (l_Score > l_BestScore)
            {
                l_BestScore = l_Score;
                l_Best = l_Candidate;
            }
        }

        if (l_Best == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("VulkanPhysicalDevice: no suitable GPU found (dynamic rendering and required features missing on all devices)");
            return false;
        }

        m_Device = l_Best;
        m_QueueFamilies = FindQueueFamilies(m_Device, surface);
        vkGetPhysicalDeviceProperties(m_Device, &m_Properties);
        m_Name = m_Properties.deviceName;

        const char* l_TypeName = "Other";
        switch (m_Properties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                l_TypeName = "Discrete";
                break;
            
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                l_TypeName = "Integrated";
                break;
            
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                l_TypeName = "Virtual";
                break;
            
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                l_TypeName = "CPU";
                break;
            
            default:
                break;
        }

        TR_CORE_INFO("VulkanPhysicalDevice: selected '{}' ({})", m_Name, l_TypeName);
        return true;
    }

    bool VulkanPhysicalDevice::IsSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const VulkanFeatures& required) const
    {
        QueueFamilyIndices l_Indices = FindQueueFamilies(device, surface);
        if (!l_Indices.IsComplete())
        {
            return false;
        }

        if (!CheckExtensionSupport(device))
        {
            return false;
        }

        if (!CheckFeatureSupport(device, required))
        {
            return false;
        }

        uint32_t l_FormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &l_FormatCount, nullptr);
        uint32_t l_PresentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &l_PresentModeCount, nullptr);

        return l_FormatCount > 0 && l_PresentModeCount > 0;
    }

    QueueFamilyIndices VulkanPhysicalDevice::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const
    {
        QueueFamilyIndices l_Indices;

        uint32_t l_FamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_FamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> l_Families(l_FamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_FamilyCount, l_Families.data());

        for (uint32_t l_Index = 0; l_Index < l_FamilyCount; l_Index++)
        {
            if (l_Families[l_Index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                l_Indices.Graphics = l_Index;
            }

            VkBool32 l_PresentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, l_Index, surface, &l_PresentSupport);
            if (l_PresentSupport)
            {
                l_Indices.Present = l_Index;
            }

            if (l_Indices.IsComplete())
            {
                break;
            }
        }

        return l_Indices;
    }

    bool VulkanPhysicalDevice::CheckExtensionSupport(VkPhysicalDevice device) const
    {
        uint32_t l_ExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &l_ExtensionCount, nullptr);

        std::vector<VkExtensionProperties> l_Available(l_ExtensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &l_ExtensionCount, l_Available.data());

        std::set<std::string> l_Required(s_RequiredDeviceExtensions.begin(), s_RequiredDeviceExtensions.end());
        for (const VkExtensionProperties& l_Extension : l_Available)
        {
            l_Required.erase(l_Extension.extensionName);
        }

        return l_Required.empty();
    }

    bool VulkanPhysicalDevice::CheckFeatureSupport(VkPhysicalDevice device, const VulkanFeatures& required) const
    {
        VkPhysicalDeviceVulkan13Features l_Vk13Features{};
        l_Vk13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        VkPhysicalDeviceVulkan11Features l_Vk11Features{};
        l_Vk11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        l_Vk11Features.pNext = &l_Vk13Features;

        VkPhysicalDeviceVulkan12Features l_Vk12Features{};
        l_Vk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        l_Vk12Features.pNext = &l_Vk11Features;

        VkPhysicalDeviceFeatures2 l_Features{};
        l_Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        l_Features.pNext = &l_Vk12Features;

        vkGetPhysicalDeviceFeatures2(device, &l_Features);

        if (required.DynamicRendering && !l_Vk13Features.dynamicRendering)
        {
            return false;
        }

        if (required.Synchronization2 && !l_Vk13Features.synchronization2)
        {
            return false;
        }

        if (required.TimelineSemaphores && !l_Vk12Features.timelineSemaphore)
        {
            return false;
        }

        if (required.ShaderDrawParameters && !l_Vk11Features.shaderDrawParameters)
        {
            return false;
        }

        return true;
    }

    uint32_t VulkanPhysicalDevice::ScoreDevice(VkPhysicalDevice device) const
    {
        VkPhysicalDeviceProperties l_Properties{};
        vkGetPhysicalDeviceProperties(device, &l_Properties);

        uint32_t l_Score = 0;

        switch (l_Properties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                l_Score += 1000;
                break;
            
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                l_Score += 100;
                break;
            
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                l_Score += 10;
                break;
            
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                l_Score += 1;
                break;
            
            default:
                break;
        }

        l_Score += l_Properties.limits.maxImageDimension2D;

        return l_Score;
    }
}