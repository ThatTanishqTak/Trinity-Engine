#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Renderer/Vulkan/VulkanInstance.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

#include <set>
#include <string>
#include <vector>

namespace Trinity
{
    static const std::vector<const char*> s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
    static const std::vector<const char*> s_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    void VulkanDevice::Initialize(const VulkanInstance& instance, bool enableValidation)
    {
        m_Instance = &instance;

        PickPhysicalDevice();
        CreateLogicalDevice(enableValidation);
    }

    void VulkanDevice::Shutdown()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            TR_CORE_TRACE("Destroying Vulkan Logical Device");

            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;

            TR_CORE_TRACE("Vulkan Logical Device Destroyed");
        }

        m_Instance = nullptr;
    }

    VkInstance VulkanDevice::GetInstance() const
    {
        return m_Instance ? m_Instance->GetInstance() : VK_NULL_HANDLE;
    }

    VkSurfaceKHR VulkanDevice::GetSurface() const
    {
        return m_Instance ? m_Instance->GetSurface() : VK_NULL_HANDLE;
    }

    void VulkanDevice::PickPhysicalDevice()
    {
        TR_CORE_TRACE("Picking Suitable GPU");

        VkInstance l_Instance = GetInstance();

        uint32_t l_DeviceCount = 0;
        vkEnumeratePhysicalDevices(l_Instance, &l_DeviceCount, nullptr);

        if (l_DeviceCount == 0)
        {
            TR_CORE_CRITICAL("No GPUs with Vulkan support found");
            std::abort();
        }

        std::vector<VkPhysicalDevice> l_Devices(l_DeviceCount);
        vkEnumeratePhysicalDevices(l_Instance, &l_DeviceCount, l_Devices.data());

        // Prefer discrete GPU
        for (const auto& l_Device : l_Devices)
        {
            VkPhysicalDeviceProperties l_Properties;
            vkGetPhysicalDeviceProperties(l_Device, &l_Properties);

            if (IsDeviceSuitable(l_Device) && l_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                m_PhysicalDevice = l_Device;
                m_Properties = l_Properties;
                m_QueueFamilyIndices = FindQueueFamilies(l_Device);

                TR_CORE_TRACE("Selected Descrete GPU: {}", l_Properties.deviceName);

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

                TR_CORE_TRACE("Selected Fallback GPU: {}");

                return;
            }
        }

        TR_CORE_CRITICAL("No suitable GPU found");
        std::abort();
    }

    void VulkanDevice::CreateLogicalDevice(bool enableValidation)
    {
        TR_CORE_TRACE("Creating Logical Device");

        std::set<uint32_t> l_UniqueQueueFamilies;
        l_UniqueQueueFamilies.insert(m_QueueFamilyIndices.GraphicsFamily.value());
        l_UniqueQueueFamilies.insert(m_QueueFamilyIndices.PresentFamily.value());
        if (m_QueueFamilyIndices.ComputeFamily.has_value())
        {
            l_UniqueQueueFamilies.insert(m_QueueFamilyIndices.ComputeFamily.value());
        }
        if (m_QueueFamilyIndices.TransferFamily.has_value())
        {
            l_UniqueQueueFamilies.insert(m_QueueFamilyIndices.TransferFamily.value());
        }

        float l_QueuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> l_QueueCreateInfos;

        for (uint32_t it_QueueFamily : l_UniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo l_QueueCreateInfo{};
            l_QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            l_QueueCreateInfo.queueFamilyIndex = it_QueueFamily;
            l_QueueCreateInfo.queueCount = 1;
            l_QueueCreateInfo.pQueuePriorities = &l_QueuePriority;
            l_QueueCreateInfos.push_back(l_QueueCreateInfo);
        }

        VkPhysicalDeviceVulkan13Features l_Features13Supported{};
        l_Features13Supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        VkPhysicalDeviceVulkan12Features l_Features12Supported{};
        l_Features12Supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        l_Features12Supported.pNext = &l_Features13Supported;

        VkPhysicalDeviceFeatures2 l_FeaturesSupported{};
        l_FeaturesSupported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        l_FeaturesSupported.pNext = &l_Features12Supported;

        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &l_FeaturesSupported);

        struct RequiredFeature
        {
            const char* Name;
            bool Supported;
        };

        const RequiredFeature l_Required[] =
        {
            { "samplerAnisotropy", l_FeaturesSupported.features.samplerAnisotropy == VK_TRUE },
            { "fillModeNonSolid", l_FeaturesSupported.features.fillModeNonSolid == VK_TRUE },
            { "wideLines", l_FeaturesSupported.features.wideLines == VK_TRUE },
            { "geometryShader", l_FeaturesSupported.features.geometryShader == VK_TRUE },
            { "shaderInt64", l_FeaturesSupported.features.shaderInt64 == VK_TRUE },
            { "multiDrawIndirect", l_FeaturesSupported.features.multiDrawIndirect == VK_TRUE },
            { "drawIndirectFirstInstance", l_FeaturesSupported.features.drawIndirectFirstInstance == VK_TRUE },
            { "bufferDeviceAddress", l_Features12Supported.bufferDeviceAddress == VK_TRUE },
            { "descriptorIndexing", l_Features12Supported.descriptorIndexing == VK_TRUE },
            { "descriptorBindingSampledImageUpdateAfterBind", l_Features12Supported.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE },
            { "descriptorBindingPartiallyBound", l_Features12Supported.descriptorBindingPartiallyBound == VK_TRUE },
            { "descriptorBindingVariableDescriptorCount", l_Features12Supported.descriptorBindingVariableDescriptorCount == VK_TRUE },
            { "runtimeDescriptorArray", l_Features12Supported.runtimeDescriptorArray == VK_TRUE },
            { "shaderSampledImageArrayNonUniformIndexing", l_Features12Supported.shaderSampledImageArrayNonUniformIndexing == VK_TRUE },
            { "scalarBlockLayout", l_Features12Supported.scalarBlockLayout == VK_TRUE },
            { "timelineSemaphore", l_Features12Supported.timelineSemaphore == VK_TRUE },
            { "hostQueryReset", l_Features12Supported.hostQueryReset == VK_TRUE },
            { "drawIndirectCount", l_Features12Supported.drawIndirectCount == VK_TRUE },
            { "synchronization2", l_Features13Supported.synchronization2 == VK_TRUE },
            { "dynamicRendering", l_Features13Supported.dynamicRendering == VK_TRUE },
        };

        bool l_AllSupported = true;
        for (const auto& it_Feature : l_Required)
        {
            if (!it_Feature.Supported)
            {
                TR_CORE_CRITICAL("Required Vulkan device feature not supported: {}", it_Feature.Name);
                l_AllSupported = false;
            }
        }

        if (!l_AllSupported)
        {
            TR_CORE_CRITICAL("Selected GPU does not satisfy Trinity-Engine's required Vulkan 1.3 feature set");
            std::abort();
        }

        VkPhysicalDeviceVulkan13Features l_Features13{};
        l_Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        l_Features13.synchronization2 = VK_TRUE;
        l_Features13.dynamicRendering = VK_TRUE;

        VkPhysicalDeviceVulkan12Features l_Features12{};
        l_Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        l_Features12.bufferDeviceAddress = VK_TRUE;
        l_Features12.descriptorIndexing = VK_TRUE;
        l_Features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        l_Features12.descriptorBindingPartiallyBound = VK_TRUE;
        l_Features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
        l_Features12.runtimeDescriptorArray = VK_TRUE;
        l_Features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        l_Features12.scalarBlockLayout = VK_TRUE;
        l_Features12.timelineSemaphore = VK_TRUE;
        l_Features12.hostQueryReset = VK_TRUE;
        l_Features12.drawIndirectCount = VK_TRUE;
        l_Features12.pNext = &l_Features13;

        VkPhysicalDeviceFeatures2 l_Features2{};
        l_Features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        l_Features2.features.samplerAnisotropy = VK_TRUE;
        l_Features2.features.fillModeNonSolid = VK_TRUE;
        l_Features2.features.wideLines = VK_TRUE;
        l_Features2.features.geometryShader = VK_TRUE;
        l_Features2.features.shaderInt64 = VK_TRUE;
        l_Features2.features.multiDrawIndirect = VK_TRUE;
        l_Features2.features.drawIndirectFirstInstance = VK_TRUE;
        l_Features2.pNext = &l_Features12;

        VkDeviceCreateInfo l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        l_CreateInfo.queueCreateInfoCount = static_cast<uint32_t>(l_QueueCreateInfos.size());
        l_CreateInfo.pQueueCreateInfos = l_QueueCreateInfos.data();
        l_CreateInfo.pEnabledFeatures = nullptr;
        l_CreateInfo.enabledExtensionCount = static_cast<uint32_t>(s_DeviceExtensions.size());
        l_CreateInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();
        l_CreateInfo.pNext = &l_Features2;

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

        if (m_QueueFamilyIndices.ComputeFamily.has_value())
        {
            vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.ComputeFamily.value(), 0, &m_ComputeQueue);
            m_HasDedicatedCompute = true;
        }
        else
        {
            m_ComputeQueue = m_GraphicsQueue;
            m_HasDedicatedCompute = false;
        }

        if (m_QueueFamilyIndices.TransferFamily.has_value())
        {
            vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.TransferFamily.value(), 0, &m_TransferQueue);
            m_HasDedicatedTransfer = true;
        }
        else
        {
            m_TransferQueue = m_GraphicsQueue;
            m_HasDedicatedTransfer = false;
        }

        m_HasTimelineSemaphore = true;

        TR_CORE_TRACE("Logical Device Created (Graphics Family = {}, Compute Family = {}, Transfer Family = {}, Dedicated Compute = {}, Dedicated Transfer = {}, Timeline Semaphore = {})", m_QueueFamilyIndices.GraphicsFamily.value(), GetComputeQueueFamily(), GetTransferQueueFamily(), m_HasDedicatedCompute, m_HasDedicatedTransfer, m_HasTimelineSemaphore);
    }

    QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) const
    {
        QueueFamilyIndices l_Indices;

        VkSurfaceKHR l_Surface = GetSurface();

        uint32_t l_QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_QueueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> l_QueueFamilies(l_QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_QueueFamilyCount, l_QueueFamilies.data());

        for (uint32_t i = 0; i < l_QueueFamilyCount; i++)
        {
            const VkQueueFlags l_Flags = l_QueueFamilies[i].queueFlags;

            const bool l_HasGraphics = (l_Flags & VK_QUEUE_GRAPHICS_BIT) != 0;
            const bool l_HasCompute = (l_Flags & VK_QUEUE_COMPUTE_BIT) != 0;
            const bool l_HasTransfer = (l_Flags & VK_QUEUE_TRANSFER_BIT) != 0;

            if (l_HasGraphics && !l_Indices.GraphicsFamily.has_value())
            {
                l_Indices.GraphicsFamily = i;
            }

            VkBool32 l_PresentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, l_Surface, &l_PresentSupport);
            if (l_PresentSupport && !l_Indices.PresentFamily.has_value())
            {
                l_Indices.PresentFamily = i;
            }

            if (l_HasCompute && !l_HasGraphics && !l_Indices.ComputeFamily.has_value())
            {
                l_Indices.ComputeFamily = i;
            }

            if (l_HasTransfer && !l_HasGraphics && !l_HasCompute && !l_Indices.TransferFamily.has_value())
            {
                l_Indices.TransferFamily = i;
            }
        }

        if (!l_Indices.TransferFamily.has_value())
        {
            for (uint32_t i = 0; i < l_QueueFamilyCount; i++)
            {
                const VkQueueFlags l_Flags = l_QueueFamilies[i].queueFlags;
                if ((l_Flags & VK_QUEUE_TRANSFER_BIT) && !(l_Flags & VK_QUEUE_GRAPHICS_BIT))
                {
                    l_Indices.TransferFamily = i;
                    break;
                }
            }
        }

        return l_Indices;
    }

    SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport() const
    {
        SwapchainSupportDetails l_Details;

        VkSurfaceKHR l_Surface = GetSurface();

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, l_Surface, &l_Details.Capabilities);

        uint32_t l_FormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, l_Surface, &l_FormatCount, nullptr);
        if (l_FormatCount != 0)
        {
            l_Details.Formats.resize(l_FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, l_Surface, &l_FormatCount, l_Details.Formats.data());
        }

        uint32_t l_PresentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, l_Surface, &l_PresentModeCount, nullptr);
        if (l_PresentModeCount != 0)
        {
            l_Details.PresentModes.resize(l_PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, l_Surface, &l_PresentModeCount, l_Details.PresentModes.data());
        }

        return l_Details;
    }

    bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device) const
    {
        QueueFamilyIndices l_Indices = FindQueueFamilies(device);
        bool l_ExtensionsSupported = CheckDeviceExtensionSupport(device);

        VkSurfaceKHR l_Surface = GetSurface();

        bool l_SwapchainAdequate = false;
        if (l_ExtensionsSupported)
        {
            SwapchainSupportDetails l_SwapchainSupport;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, l_Surface, &l_SwapchainSupport.Capabilities);

            uint32_t l_FormatCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, l_Surface, &l_FormatCount, nullptr);
            l_SwapchainSupport.Formats.resize(l_FormatCount);
            if (l_FormatCount > 0)
            {
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, l_Surface, &l_FormatCount, l_SwapchainSupport.Formats.data());
            }

            uint32_t l_PresentModeCount = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, l_Surface, &l_PresentModeCount, nullptr);

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
}