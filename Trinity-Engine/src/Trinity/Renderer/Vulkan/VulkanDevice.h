#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace Trinity
{
    class VulkanContext;

    class VulkanDevice
    {
    public:
        void Initialize(const VulkanContext& context);
        void Shutdown();

        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkDevice GetDevice() const { return m_Device; }

        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }
        VkQueue GetComputeQueue() const { return m_ComputeQueue; }
        VkQueue GetTransferQueue() const { return m_TransferQueue; }

        uint32_t GetGraphicsQueueFamilyIndex() const { return m_GraphicsQueueFamilyIndex.value(); }
        uint32_t GetPresentQueueFamilyIndex() const { return m_PresentQueueFamilyIndex.value(); }
        uint32_t GetComputeQueueFamilyIndex() const { return m_ComputeQueueFamilyIndex.value(); }
        uint32_t GetTransferQueueFamilyIndex() const { return m_TransferQueueFamilyIndex.value(); }

    private:
        struct QueueFamilyIndices
        {
            std::optional<uint32_t> Graphics;
            std::optional<uint32_t> Present;
            std::optional<uint32_t> Compute;
            std::optional<uint32_t> Transfer;

            bool IsComplete() const { return Graphics.has_value() && Present.has_value(); }
        };

        void PickPhysicalDevice(const VulkanContext& context);
        void CreateLogicalDevice(const VulkanContext& context);

        void ReleasePhysicalDevice();
        void DestroyLogicalDevice();

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;
        bool HasSwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;
        bool AreAllExtensionsSupported(VkPhysicalDevice device, const std::vector<const char*>& required) const;

    private:
        VkAllocationCallbacks* m_Allocator = nullptr;

        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_PhysicalDeviceProperties{};
        VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures{};
        VkPhysicalDeviceVulkan13Features m_Vulkan13Features{};

        VkDevice m_Device = VK_NULL_HANDLE;

        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;
        VkQueue m_ComputeQueue = VK_NULL_HANDLE;
        VkQueue m_TransferQueue = VK_NULL_HANDLE;

        std::optional<uint32_t> m_GraphicsQueueFamilyIndex;
        std::optional<uint32_t> m_PresentQueueFamilyIndex;
        std::optional<uint32_t> m_ComputeQueueFamilyIndex;
        std::optional<uint32_t> m_TransferQueueFamilyIndex;
    };
}