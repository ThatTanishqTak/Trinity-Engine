#include <Trinity/Renderer/Backends/Vulkan/VulkanDevice.h>

#include <set>
#include <vector>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    VulkanDevice::~VulkanDevice()
    {
        Shutdown();
    }

    bool VulkanDevice::Initialize(const NativeWindowHandle& window, const std::string& applicationName, bool enableValidation)
    {
        if (!m_Instance.Initialize(applicationName, enableValidation))
        {
            return false;
        }

        if (!m_Surface.Initialize(m_Instance.GetHandle(), window))
        {
            return false;
        }

        VulkanFeatures l_Required;
        if (!m_PhysicalDevice.Select(m_Instance.GetHandle(), m_Surface.GetHandle(), l_Required))
        {
            return false;
        }

        if (!CreateLogicalDevice())
        {
            return false;
        }

        if (!m_Allocator.Initialize(m_Instance.GetHandle(), m_PhysicalDevice.GetHandle(), m_Device))
        {
            return false;
        }

        if (!m_Commands.Initialize(m_Device, m_GraphicsQueueFamily, m_GraphicsQueue))
        {
            return false;
        }

        TR_CORE_INFO("VulkanDevice: initialized");
        return true;
    }

    void VulkanDevice::Shutdown()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device);

            if (m_Swapchain != nullptr)
            {
                m_Swapchain->Shutdown();
                m_Swapchain.reset();
            }

            m_Commands.Shutdown();
            m_Allocator.Shutdown();

            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        m_Surface.Shutdown();
        m_Instance.Shutdown();
    }

    bool VulkanDevice::CreateLogicalDevice()
    {
        const QueueFamilyIndices& l_Families = m_PhysicalDevice.GetQueueFamilies();
        m_GraphicsQueueFamily = l_Families.Graphics.value();
        m_PresentQueueFamily = l_Families.Present.value();

        std::set<uint32_t> l_UniqueFamilies = { m_GraphicsQueueFamily, m_PresentQueueFamily };

        float l_Priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> l_QueueInfos;
        l_QueueInfos.reserve(l_UniqueFamilies.size());

        for (uint32_t l_Family : l_UniqueFamilies)
        {
            VkDeviceQueueCreateInfo l_QueueCreateInfo{};
            l_QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            l_QueueCreateInfo.queueFamilyIndex = l_Family;
            l_QueueCreateInfo.queueCount = 1;
            l_QueueCreateInfo.pQueuePriorities = &l_Priority;
            l_QueueInfos.push_back(l_QueueCreateInfo);
        }

        VkPhysicalDeviceVulkan13Features l_Vulkan13Features{};
        l_Vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        l_Vulkan13Features.dynamicRendering = VK_TRUE;
        l_Vulkan13Features.synchronization2 = VK_TRUE;

        VkPhysicalDeviceVulkan12Features l_Vulkan12Features{};
        l_Vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        l_Vulkan12Features.timelineSemaphore = VK_TRUE;
        l_Vulkan12Features.pNext = &l_Vulkan13Features;

        VkPhysicalDeviceFeatures2 l_Features{};
        l_Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        l_Features.pNext = &l_Vulkan12Features;

        const std::vector<const char*>& l_Extensions = m_PhysicalDevice.GetRequiredExtensions();

        VkDeviceCreateInfo l_DeviceCreateInfo{};
        l_DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        l_DeviceCreateInfo.pNext = &l_Features;
        l_DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(l_QueueInfos.size());
        l_DeviceCreateInfo.pQueueCreateInfos = l_QueueInfos.data();
        l_DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(l_Extensions.size());
        l_DeviceCreateInfo.ppEnabledExtensionNames = l_Extensions.data();

        VkResult l_Result = vkCreateDevice(m_PhysicalDevice.GetHandle(), &l_DeviceCreateInfo, nullptr, &m_Device);
        if (l_Result != VK_SUCCESS)
        {
            TR_CORE_CRITICAL("VulkanDevice: vkCreateDevice failed ({})", static_cast<int>(l_Result));
            return false;
        }

        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_PresentQueueFamily, 0, &m_PresentQueue);

        return true;
    }

    bool VulkanDevice::CreateSwapchain(const SwapchainDescription& description)
    {
        m_Swapchain = std::make_unique<VulkanSwapchain>(*this, description);
        if (!m_Swapchain->Initialize())
        {
            m_Swapchain.reset();
            return false;
        }

        return true;
    }
}