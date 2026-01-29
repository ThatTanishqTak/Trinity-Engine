#include "Engine/Renderer/Vulkan/VulkanDevice.h"

#include "Engine/Renderer/Vulkan/VulkanSwapchain.h"
#include "Engine/Utilities/Utilities.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>
#include <cstdlib>

namespace
{
    const std::vector<const char*> s_DeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
}

namespace Engine
{
    void VulkanDevice::Initialize(GLFWwindow* nativeWindow)
    {
        m_NativeWindow = nativeWindow;

        if (!m_NativeWindow)
        {
            TR_CORE_CRITICAL("VulkanDevice: Native window handle is null (expected GLFWwindow*).");

            std::abort();
        }

        m_Context.Initialize(m_NativeWindow);
        PickPhysicalDevice();
        CreateLogicalDevice();
    }

    void VulkanDevice::Shutdown()
    {
        if (m_Device)
        {
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        m_Context.Shutdown();
        m_PhysicalDevice = VK_NULL_HANDLE;
        m_GraphicsQueue = VK_NULL_HANDLE;
        m_PresentQueue = VK_NULL_HANDLE;
        m_QueueFamilyIndices = {};
        m_NativeWindow = nullptr;
    }

    VulkanDevice::QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) const
    {
        QueueFamilyIndices l_Indices;

        uint32_t l_Count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_Count, nullptr);

        std::vector<VkQueueFamilyProperties> l_Families(l_Count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_Count, l_Families.data());

        uint32_t l_Index = 0;
        for (const auto& it_Family : l_Families)
        {
            if (it_Family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                l_Indices.GraphicsFamily = l_Index;
            }

            VkBool32 l_PresentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, l_Index, m_Context.GetSurface(), &l_PresentSupport);
            if (l_PresentSupport)
            {
                l_Indices.PresentFamily = l_Index;
            }

            if (l_Indices.IsComplete())
            {
                break;
            }

            ++l_Index;
        }

        return l_Indices;
    }

    bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
    {
        uint32_t l_Count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &l_Count, nullptr);

        std::vector<VkExtensionProperties> l_Available(l_Count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &l_Count, l_Available.data());

        std::set<std::string> l_Required(s_DeviceExtensions.begin(), s_DeviceExtensions.end());
        for (const auto& it_Ext : l_Available)
        {
            l_Required.erase(it_Ext.extensionName);
        }

        return l_Required.empty();
    }

    uint32_t VulkanDevice::RateDeviceSuitability(VkPhysicalDevice device) const
    {
        QueueFamilyIndices l_Indices = FindQueueFamilies(device);
        if (!l_Indices.IsComplete())
        {
            return 0;
        }

        if (!CheckDeviceExtensionSupport(device))
        {
            return 0;
        }

        const auto l_Support = Engine::VulkanSwapchain::QuerySupport(device, m_Context.GetSurface());
        if (l_Support.Formats.empty() || l_Support.PresentModes.empty())
        {
            return 0;
        }

        VkPhysicalDeviceProperties l_Props{};
        vkGetPhysicalDeviceProperties(device, &l_Props);

        VkPhysicalDeviceMemoryProperties l_MemProps{};
        vkGetPhysicalDeviceMemoryProperties(device, &l_MemProps);

        VkDeviceSize l_DeviceLocalBytes = 0;
        for (uint32_t it_Heap = 0; it_Heap < l_MemProps.memoryHeapCount; ++it_Heap)
        {
            const VkMemoryHeap& l_Heap = l_MemProps.memoryHeaps[it_Heap];
            if (l_Heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                l_DeviceLocalBytes = std::max(l_DeviceLocalBytes, l_Heap.size);
            }
        }

        uint32_t l_Score = 0;

        switch (l_Props.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   l_Score += 100000; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: l_Score += 10000;  break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    l_Score += 1000;   break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            l_Score += 100;    break;
            default: break;
        }

        l_Score += static_cast<uint32_t>(l_DeviceLocalBytes / (1024ull * 1024ull * 1024ull)) * 1000u;
        l_Score += l_Props.limits.maxImageDimension2D;

        return l_Score;
    }

    void VulkanDevice::PickPhysicalDevice()
    {
        uint32_t l_Count = 0;
        vkEnumeratePhysicalDevices(m_Context.GetInstance(), &l_Count, nullptr);

        if (l_Count == 0)
        {
            TR_CORE_CRITICAL("No Vulkan-compatible GPUs found.");

            std::abort();
        }

        std::vector<VkPhysicalDevice> l_Devices(l_Count);
        vkEnumeratePhysicalDevices(m_Context.GetInstance(), &l_Count, l_Devices.data());

        VkPhysicalDevice l_BestDevice = VK_NULL_HANDLE;
        uint32_t l_BestScore = 0;

        for (VkPhysicalDevice it_Device : l_Devices)
        {
            uint32_t l_Score = RateDeviceSuitability(it_Device);
            if (l_Score > l_BestScore)
            {
                l_BestScore = l_Score;
                l_BestDevice = it_Device;
            }
        }

        m_PhysicalDevice = l_BestDevice;

        if (m_PhysicalDevice == VK_NULL_HANDLE || l_BestScore == 0)
        {
            TR_CORE_CRITICAL("Failed to find a suitable GPU.");

            std::abort();
        }

        VkPhysicalDeviceProperties l_SelectedProps{};
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &l_SelectedProps);
        TR_CORE_TRACE("Vulkan selected GPU: {}", l_SelectedProps.deviceName);
    }

    void VulkanDevice::CreateLogicalDevice()
    {
        m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> l_QueueInfos;
        std::set<uint32_t> l_UniqueFamilies =
        {
            m_QueueFamilyIndices.GraphicsFamily.value(),
            m_QueueFamilyIndices.PresentFamily.value()
        };

        float l_Priority = 1.0f;
        for (uint32_t it_Family : l_UniqueFamilies)
        {
            VkDeviceQueueCreateInfo l_Q{};
            l_Q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            l_Q.queueFamilyIndex = it_Family;
            l_Q.queueCount = 1;
            l_Q.pQueuePriorities = &l_Priority;
            l_QueueInfos.push_back(l_Q);
        }

        VkPhysicalDeviceFeatures l_Features{};

        VkDeviceCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        l_Info.queueCreateInfoCount = static_cast<uint32_t>(l_QueueInfos.size());
        l_Info.pQueueCreateInfos = l_QueueInfos.data();
        l_Info.pEnabledFeatures = &l_Features;
        l_Info.enabledExtensionCount = static_cast<uint32_t>(s_DeviceExtensions.size());
        l_Info.ppEnabledExtensionNames = s_DeviceExtensions.data();

        if (m_Context.UseValidationLayers())
        {
            const std::vector<const char*>& l_ValidationLayers = VulkanContext::GetValidationLayers();
            l_Info.enabledLayerCount = static_cast<uint32_t>(l_ValidationLayers.size());
            l_Info.ppEnabledLayerNames = l_ValidationLayers.data();
        }

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateDevice(m_PhysicalDevice, &l_Info, nullptr, &m_Device), "vkCreateDevice");

        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.PresentFamily.value(), 0, &m_PresentQueue);
    }
}