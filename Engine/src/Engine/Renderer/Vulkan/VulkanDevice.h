#pragma once

#include "Engine/Renderer/Vulkan/VulkanContext.h"

#include <vulkan/vulkan.h>

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Engine
{
    class VulkanDebugUtils;

    class VulkanDevice
    {
    public:
        VulkanDevice() = default;
        ~VulkanDevice() = default;

        VulkanDevice(const VulkanDevice&) = delete;
        VulkanDevice& operator=(const VulkanDevice&) = delete;
        VulkanDevice(VulkanDevice&&) = delete;
        VulkanDevice& operator=(VulkanDevice&&) = delete;

        void Initialize(VulkanContext& context);
        void Shutdown();

        void SetDebugUtils(VulkanDebugUtils* debugUtils) { m_DebugUtils = debugUtils; }
        const VulkanDebugUtils* GetDebugUtils() const { return m_DebugUtils; }

        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkDevice GetDevice() const { return m_Device; }
        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }

        uint32_t GetGraphicsQueueFamily() const { return m_QueueFamilies.GraphicsFamily.value(); }
        uint32_t GetPresentQueueFamily() const { return m_QueueFamilies.PresentFamily.value(); }

        struct QueueFamilyIndices
        {
            std::optional<uint32_t> GraphicsFamily;
            std::optional<uint32_t> PresentFamily;

            bool IsComplete() const { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
        };

        struct SwapchainSupportDetails
        {
            VkSurfaceCapabilitiesKHR Capabilities{};
            std::vector<VkSurfaceFormatKHR> Formats;
            std::vector<VkPresentModeKHR> PresentModes;
        };

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
        SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device) const;

    private:
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        bool IsDeviceSuitable(VkPhysicalDevice device) const;
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;

    private:
        VulkanContext* m_Context = nullptr;
        VulkanDebugUtils* m_DebugUtils = nullptr;

        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;

        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;

        QueueFamilyIndices m_QueueFamilies{};

        const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };
}