#pragma once

#include <memory>
#include <string>

#include <vulkan/vulkan.h>

#include <Trinity/Platform/PlatformTypes.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanInstance.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanSurface.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanPhysicalDevice.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanAllocator.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanCommands.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanSwapchain.h>

namespace Trinity
{
    class VulkanDevice
    {
    public:
        VulkanDevice() = default;
        ~VulkanDevice();

        VulkanDevice(const VulkanDevice&) = delete;
        VulkanDevice& operator=(const VulkanDevice&) = delete;

        bool Initialize(const NativeWindowHandle& window, const std::string& applicationName, bool enableValidation);
        void Shutdown();

        VkDevice GetHandle() const { return m_Device; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice.GetHandle(); }
        VkInstance GetInstance() const { return m_Instance.GetHandle(); }
        VkSurfaceKHR GetSurface() const { return m_Surface.GetHandle(); }
        VulkanAllocator& GetAllocator() { return m_Allocator; }
        VulkanCommands& GetCommands() { return m_Commands; }
        VulkanSwapchain& GetSwapchain() { return *m_Swapchain; }

        bool CreateSwapchain(const SwapchainDescription& description);
        bool HasSwapchain() const { return m_Swapchain != nullptr; }

        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }
        uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
        uint32_t GetPresentQueueFamily() const {return m_PresentQueueFamily; }

    private:
        bool CreateLogicalDevice();

    private:
        VulkanInstance m_Instance;
        VulkanSurface m_Surface;
        VulkanPhysicalDevice m_PhysicalDevice;
        VulkanAllocator m_Allocator;
        VulkanCommands m_Commands;
        std::unique_ptr<VulkanSwapchain> m_Swapchain;

        VkDevice m_Device = VK_NULL_HANDLE;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;

        uint32_t m_GraphicsQueueFamily = 0;
        uint32_t m_PresentQueueFamily = 0;
    };
}