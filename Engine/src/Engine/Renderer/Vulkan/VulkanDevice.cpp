#include "Engine/Renderer/Vulkan/VulkanDevice.h"

#include "Engine/Utilities/Utilities.h"

#include <stdexcept>
#include <string>

namespace Engine
{
    static void VKCheckStrict(VkResult result, const char* what)
    {
        if (result != VK_SUCCESS)
        {
            TR_CORE_CRITICAL("Vulkan failure: {} (VkResult={})", what, (int)result);
            throw std::runtime_error(std::string(what) + " failed.");
        }
    }

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
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Context->GetInstance(), &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            TR_CORE_CRITICAL("No Vulkan GPUs found.");
            throw std::runtime_error("No Vulkan GPU found.");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Context->GetInstance(), &deviceCount, devices.data());

        for (auto device : devices)
        {
            if (IsDeviceSuitable(device))
            {
                m_PhysicalDevice = device;
                m_QueueFamilies = FindQueueFamilies(device);
                break;
            }
        }

        if (m_PhysicalDevice == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("Failed to find a suitable GPU.");
            throw std::runtime_error("No suitable Vulkan GPU.");
        }
    }

    bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device) const
    {
        const auto indices = FindQueueFamilies(device);
        if (!indices.IsComplete())
            return false;

        if (!CheckDeviceExtensionSupport(device))
            return false;

        const auto swapSupport = QuerySwapchainSupport(device);
        if (swapSupport.Formats.empty() || swapSupport.PresentModes.empty())
            return false;

        return true;
    }

    bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> available(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available.data());

        std::set<std::string> required(m_DeviceExtensions.begin(), m_DeviceExtensions.end());
        for (const auto& ext : available)
            required.erase(ext.extensionName);

        return required.empty();
    }

    VulkanDevice::QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) const
    {
        QueueFamilyIndices indices{};

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, families.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.GraphicsFamily = i;

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Context->GetSurface(), &presentSupport);
            if (presentSupport)
                indices.PresentFamily = i;

            if (indices.IsComplete())
                break;
        }

        return indices;
    }

    VulkanDevice::SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport(VkPhysicalDevice device) const
    {
        SwapchainSupportDetails details{};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Context->GetSurface(), &details.Capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Context->GetSurface(), &formatCount, nullptr);
        if (formatCount)
        {
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Context->GetSurface(), &formatCount, details.Formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Context->GetSurface(), &presentModeCount, nullptr);
        if (presentModeCount)
        {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Context->GetSurface(), &presentModeCount, details.PresentModes.data());
        }

        return details;
    }

    void VulkanDevice::CreateLogicalDevice()
    {
        const auto indices = FindQueueFamilies(m_PhysicalDevice);

        std::set<uint32_t> uniqueFamilies = { indices.GraphicsFamily.value(), indices.PresentFamily.value() };

        float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        queueInfos.reserve(uniqueFamilies.size());

        for (uint32_t family : uniqueFamilies)
        {
            VkDeviceQueueCreateInfo q{};
            q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            q.queueFamilyIndex = family;
            q.queueCount = 1;
            q.pQueuePriorities = &queuePriority;
            queueInfos.push_back(q);
        }

        VkPhysicalDeviceFeatures features{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = (uint32_t)queueInfos.size();
        createInfo.pQueueCreateInfos = queueInfos.data();
        createInfo.pEnabledFeatures = &features;

        createInfo.enabledExtensionCount = (uint32_t)m_DeviceExtensions.size();
        createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

        // Device layers are deprecated; instance validation is enough.
        createInfo.enabledLayerCount = 0;

        VKCheckStrict(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device), "vkCreateDevice");

        vkGetDeviceQueue(m_Device, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, indices.PresentFamily.value(), 0, &m_PresentQueue);

        m_QueueFamilies = indices;
    }
}