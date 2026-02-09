#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanContext.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <vector>

namespace Trinity
{
    void VulkanDevice::Initialize(const VulkanContext& context)
    {
        TR_CORE_TRACE("Initializing Vulkan Device");

        if (m_Device != VK_NULL_HANDLE)
        {
            TR_CORE_WARN("VulkanDevice::Initialize called while already initialized");

            return;
        }

        m_Allocator = context.GetAllocator();

        PickPhysicalDevice(context);
        CreateLogicalDevice(context);

        TR_CORE_TRACE("Vulkan Device Initialized");
    }

    void VulkanDevice::Shutdown()
    {
        TR_CORE_TRACE("Shutting Down Vulkan Device");

        DestroyLogicalDevice();
        ReleasePhysicalDevice();

        TR_CORE_TRACE("Vulkan Device Shutdown Complete");
    }

    void VulkanDevice::PickPhysicalDevice(const VulkanContext& context)
    {
        TR_CORE_TRACE("Selecting Physical Device");

        uint32_t l_PhysicalDeviceCount = 0;
        Utilities::VulkanUtilities::VKCheck(vkEnumeratePhysicalDevices(context.GetInstance(), &l_PhysicalDeviceCount, nullptr), "Failed vkEnumeratePhysicalDevices");

        if (l_PhysicalDeviceCount == 0)
        {
            TR_CORE_CRITICAL("No Vulkan supported GPU(s) found");

            std::abort();
        }

        std::vector<VkPhysicalDevice> l_PhysicalDevices(l_PhysicalDeviceCount);
        Utilities::VulkanUtilities::VKCheck(vkEnumeratePhysicalDevices(context.GetInstance(), &l_PhysicalDeviceCount, l_PhysicalDevices.data()), "Failed vkEnumeratePhysicalDevices");

        VkPhysicalDevice l_BestDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties l_BestProps{};
        VkPhysicalDeviceFeatures l_BestFeatures{};
        int l_BestScore = -1;

        const std::vector<const char*> l_RequiredExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        for (const auto& it_Device : l_PhysicalDevices)
        {
            VkPhysicalDeviceProperties l_Properties{};
            vkGetPhysicalDeviceProperties(it_Device, &l_Properties);

            if (l_Properties.apiVersion < VK_API_VERSION_1_3)
            {
                continue;
            }

            VkPhysicalDeviceFeatures l_Features{};
            vkGetPhysicalDeviceFeatures(it_Device, &l_Features);

            const QueueFamilyIndices l_Indices = FindQueueFamilies(it_Device, context.GetSurface());
            if (!l_Indices.IsComplete())
            {
                continue;
            }

            if (!AreAllExtensionsSupported(it_Device, l_RequiredExtensions))
            {
                continue;
            }

            if (!HasSwapchainSupport(it_Device, context.GetSurface()))
            {
                continue;
            }

            int l_Score = 0;

            if (l_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                l_Score += 1000;
            }
            else if (l_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            {
                l_Score += 500;
            }
            else if (l_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
            {
                l_Score += 200;
            }

            if (l_Features.samplerAnisotropy)
            {
                l_Score += 250;
            }

            if (l_Features.geometryShader)
            {
                l_Score += 100;
            }

            const uint32_t l_ApiMajor = VK_VERSION_MAJOR(l_Properties.apiVersion);
            const uint32_t l_ApiMinor = VK_VERSION_MINOR(l_Properties.apiVersion);
            l_Score += static_cast<int>(l_ApiMajor) * 50 + static_cast<int>(l_ApiMinor) * 10;

            if (l_Properties.limits.maxImageDimension2D >= 8192)
            {
                l_Score += 100;
            }
            else
            {
                l_Score -= 200;
            }

            if (l_Score > l_BestScore)
            {
                l_BestScore = l_Score;
                l_BestDevice = it_Device;
                l_BestProps = l_Properties;
                l_BestFeatures = l_Features;
            }
        }

        if (l_BestDevice == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("No suitable Vulkan 1.3 physical device found");

            std::abort();
        }

        m_PhysicalDevice = l_BestDevice;
        m_PhysicalDeviceProperties = l_BestProps;
        m_PhysicalDeviceFeatures = l_BestFeatures;

        // Query supported Vulkan 1.3 features only (plus base features via Features2).
        VkPhysicalDeviceFeatures2 l_Features2{};
        l_Features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceVulkan13Features l_Vulkan13Features{};
        l_Vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        l_Features2.pNext = &l_Vulkan13Features;

        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &l_Features2);

        m_PhysicalDeviceFeatures = l_Features2.features;
        m_Vulkan13Features = l_Vulkan13Features;

        TR_CORE_TRACE("Selected Physical Device: {} (API {}.{}.{})", m_PhysicalDeviceProperties.deviceName, VK_VERSION_MAJOR(m_PhysicalDeviceProperties.apiVersion),
            VK_VERSION_MINOR(m_PhysicalDeviceProperties.apiVersion), VK_VERSION_PATCH(m_PhysicalDeviceProperties.apiVersion));
    }

    void VulkanDevice::CreateLogicalDevice(const VulkanContext& context)
    {
        TR_CORE_TRACE("Creating Logical Device");

        const QueueFamilyIndices l_Indices = FindQueueFamilies(m_PhysicalDevice, context.GetSurface());
        if (!l_Indices.IsComplete())
        {
            TR_CORE_CRITICAL("CreateLogicalDevice: missing required queue families");

            std::abort();
        }

        m_GraphicsQueueFamilyIndex = l_Indices.Graphics;
        m_PresentQueueFamilyIndex = l_Indices.Present;
        m_ComputeQueueFamilyIndex = l_Indices.Compute;
        m_TransferQueueFamilyIndex = l_Indices.Transfer;

        std::vector<uint32_t> l_UniqueFamilies{};
        auto a_PushUnique = [&l_UniqueFamilies](uint32_t familyIndex)
        {
            if (std::find(l_UniqueFamilies.begin(), l_UniqueFamilies.end(), familyIndex) == l_UniqueFamilies.end())
            {
                l_UniqueFamilies.push_back(familyIndex);
            }
        };

        a_PushUnique(m_GraphicsQueueFamilyIndex.value());
        a_PushUnique(m_PresentQueueFamilyIndex.value());
        a_PushUnique(m_ComputeQueueFamilyIndex.value());
        a_PushUnique(m_TransferQueueFamilyIndex.value());

        float l_QueuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> l_QueueCreateInfos{};
        l_QueueCreateInfos.reserve(l_UniqueFamilies.size());

        for (uint32_t it_FamilyIndex : l_UniqueFamilies)
        {
            VkDeviceQueueCreateInfo l_QueueCreateInfo{};
            l_QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            l_QueueCreateInfo.queueFamilyIndex = it_FamilyIndex;
            l_QueueCreateInfo.queueCount = 1;
            l_QueueCreateInfo.pQueuePriorities = &l_QueuePriority;
            l_QueueCreateInfos.push_back(l_QueueCreateInfo);
        }

        VkPhysicalDeviceFeatures2 l_EnabledFeatures2{};
        l_EnabledFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceVulkan13Features l_EnabledVulkan13Features{};
        l_EnabledVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        l_EnabledFeatures2.pNext = &l_EnabledVulkan13Features;

        if (m_PhysicalDeviceFeatures.samplerAnisotropy)
        {
            l_EnabledFeatures2.features.samplerAnisotropy = VK_TRUE;
        }

        l_EnabledVulkan13Features.dynamicRendering = m_Vulkan13Features.dynamicRendering;
        l_EnabledVulkan13Features.synchronization2 = m_Vulkan13Features.synchronization2;
        l_EnabledVulkan13Features.maintenance4 = m_Vulkan13Features.maintenance4;

        const std::vector<const char*> l_DeviceExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkDeviceCreateInfo l_DeviceCreateInfo{};
        l_DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        l_DeviceCreateInfo.pNext = &l_EnabledFeatures2;
        l_DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(l_QueueCreateInfos.size());
        l_DeviceCreateInfo.pQueueCreateInfos = l_QueueCreateInfos.data();
        l_DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(l_DeviceExtensions.size());
        l_DeviceCreateInfo.ppEnabledExtensionNames = l_DeviceExtensions.data();
        l_DeviceCreateInfo.pEnabledFeatures = nullptr;

        Utilities::VulkanUtilities::VKCheck(vkCreateDevice(m_PhysicalDevice, &l_DeviceCreateInfo, m_Allocator, &m_Device), "Failed vkCreateDevice");

        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamilyIndex.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_PresentQueueFamilyIndex.value(), 0, &m_PresentQueue);
        vkGetDeviceQueue(m_Device, m_ComputeQueueFamilyIndex.value(), 0, &m_ComputeQueue);
        vkGetDeviceQueue(m_Device, m_TransferQueueFamilyIndex.value(), 0, &m_TransferQueue);

        TR_CORE_TRACE("Logical Device Created");
        TR_CORE_TRACE("Queue Families: Graphics = {} Present = {} Compute = {} Transfer = {}", m_GraphicsQueueFamilyIndex.value(), m_PresentQueueFamilyIndex.value(),
            m_ComputeQueueFamilyIndex.value(), m_TransferQueueFamilyIndex.value());
    }

    void VulkanDevice::ReleasePhysicalDevice()
    {
        TR_CORE_TRACE("Releasing Physical Device");

        m_PhysicalDevice = VK_NULL_HANDLE;
        m_PhysicalDeviceProperties = {};
        m_PhysicalDeviceFeatures = {};
        m_Vulkan13Features = {};
        m_Allocator = nullptr;

        TR_CORE_TRACE("Physical Device Released");
    }

    void VulkanDevice::DestroyLogicalDevice()
    {
        TR_CORE_TRACE("Destroying Logical Device");

        if (m_Device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_Device, m_Allocator);
            m_Device = VK_NULL_HANDLE;
        }

        m_GraphicsQueue = VK_NULL_HANDLE;
        m_PresentQueue = VK_NULL_HANDLE;
        m_ComputeQueue = VK_NULL_HANDLE;
        m_TransferQueue = VK_NULL_HANDLE;
        m_GraphicsQueueFamilyIndex.reset();
        m_PresentQueueFamilyIndex.reset();
        m_ComputeQueueFamilyIndex.reset();
        m_TransferQueueFamilyIndex.reset();

        TR_CORE_TRACE("Logical Device Destroyed");
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

    VulkanDevice::QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const
    {
        if (surface == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("No valid surface provided to FindQueueFamilies");

            std::abort();
        }

        QueueFamilyIndices l_Indices{};

        uint32_t l_QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_QueueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> l_QueueFamilies(l_QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_QueueFamilyCount, l_QueueFamilies.data());

        for (uint32_t i = 0; i < l_QueueFamilyCount; ++i)
        {
            const VkQueueFamilyProperties& it_Family = l_QueueFamilies[i];
            if (it_Family.queueCount == 0)
            {
                continue;
            }

            if (it_Family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                if (!l_Indices.Graphics.has_value())
                {
                    l_Indices.Graphics = i;
                }
            }

            if (it_Family.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                const bool l_IsDedicated = (it_Family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0;
                if (!l_Indices.Compute.has_value() || l_IsDedicated)
                {
                    l_Indices.Compute = i;
                }
            }

            if (it_Family.queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                const bool l_IsDedicated = (it_Family.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0;
                if (!l_Indices.Transfer.has_value() || l_IsDedicated)
                {
                    l_Indices.Transfer = i;
                }
            }

            VkBool32 l_PresentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &l_PresentSupport);
            if (l_PresentSupport)
            {
                if (!l_Indices.Present.has_value())
                {
                    l_Indices.Present = i;
                }
            }
        }

        if (!l_Indices.Compute.has_value() && l_Indices.Graphics.has_value())
        {
            l_Indices.Compute = l_Indices.Graphics;
        }

        if (!l_Indices.Transfer.has_value() && l_Indices.Graphics.has_value())
        {
            l_Indices.Transfer = l_Indices.Graphics;
        }

        return l_Indices;
    }

    bool VulkanDevice::HasSwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const
    {
        uint32_t l_FormatCount = 0;
        Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &l_FormatCount, nullptr), "Failed vkGetPhysicalDeviceSurfaceFormatsKHR");

        uint32_t l_PresentModeCount = 0;
        Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &l_PresentModeCount, nullptr), "Failed vkGetPhysicalDeviceSurfacePresentModesKHR");

        return l_FormatCount > 0 && l_PresentModeCount > 0;
    }

    bool VulkanDevice::AreAllExtensionsSupported(VkPhysicalDevice device, const std::vector<const char*>& required) const
    {
        uint32_t l_Count = 0;
        Utilities::VulkanUtilities::VKCheck(vkEnumerateDeviceExtensionProperties(device, nullptr, &l_Count, nullptr), "Failed vkEnumerateDeviceExtensionProperties");

        std::vector<VkExtensionProperties> l_Available(l_Count);
        Utilities::VulkanUtilities::VKCheck(vkEnumerateDeviceExtensionProperties(device, nullptr, &l_Count, l_Available.data()), "Failed vkEnumerateDeviceExtensionProperties");

        for (const char* it_Name : required)
        {
            bool l_Found = false;
            for (const auto& it_Ext : l_Available)
            {
                if (strcmp(it_Ext.extensionName, it_Name) == 0)
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