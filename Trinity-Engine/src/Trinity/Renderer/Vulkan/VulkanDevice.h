#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace Trinity
{
    class VulkanInstance;

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> GraphicsFamily;
        std::optional<uint32_t> PresentFamily;
        std::optional<uint32_t> ComputeFamily;
        std::optional<uint32_t> TransferFamily;

        bool IsComplete() const
        {
            return GraphicsFamily.has_value() && PresentFamily.has_value();
        }
    };

    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities{};
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    class VulkanDevice
    {
    public:
        VulkanDevice() = default;
        ~VulkanDevice() = default;

        void Initialize(const VulkanInstance& instance, bool enableValidation);
        void Shutdown();

        VkInstance GetInstance() const;
        VkSurfaceKHR GetSurface() const;
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkDevice GetDevice() const { return m_Device; }

        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }
        VkQueue GetComputeQueue() const { return m_ComputeQueue; }
        VkQueue GetTransferQueue() const { return m_TransferQueue; }

        uint32_t GetGraphicsQueueFamily() const { return m_QueueFamilyIndices.GraphicsFamily.value_or(0); }
        uint32_t GetPresentQueueFamily() const { return m_QueueFamilyIndices.PresentFamily.value_or(0); }
        uint32_t GetComputeQueueFamily() const { return m_QueueFamilyIndices.ComputeFamily.value_or(GetGraphicsQueueFamily()); }
        uint32_t GetTransferQueueFamily() const { return m_QueueFamilyIndices.TransferFamily.value_or(GetGraphicsQueueFamily()); }

        bool HasDedicatedComputeQueue() const { return m_HasDedicatedCompute; }
        bool HasDedicatedTransferQueue() const { return m_HasDedicatedTransfer; }
        bool HasTimelineSemaphore() const { return m_HasTimelineSemaphore; }

        bool SupportsRayTracing() const { return m_HasRayTracing; }
        bool SupportsMeshShaders() const { return m_HasMeshShaders; }
        bool SupportsFragmentShadingRate() const { return m_HasFragmentShadingRate; }

        const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
        const VkPhysicalDeviceProperties& GetProperties() const { return m_Properties; }

        SwapchainSupportDetails QuerySwapchainSupport() const;
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

    private:
        void PickPhysicalDevice();
        void CreateLogicalDevice(bool enableValidation);

        bool IsDeviceSuitable(VkPhysicalDevice device) const;
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;

    private:
        const VulkanInstance* m_Instance = nullptr;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;

        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;
        VkQueue m_ComputeQueue = VK_NULL_HANDLE;
        VkQueue m_TransferQueue = VK_NULL_HANDLE;

        bool m_HasDedicatedCompute = false;
        bool m_HasDedicatedTransfer = false;
        bool m_HasTimelineSemaphore = false;

        bool m_HasRayTracing = false;
        bool m_HasMeshShaders = false;
        bool m_HasFragmentShadingRate = false;

        QueueFamilyIndices m_QueueFamilyIndices;
        VkPhysicalDeviceProperties m_Properties{};
    };
}