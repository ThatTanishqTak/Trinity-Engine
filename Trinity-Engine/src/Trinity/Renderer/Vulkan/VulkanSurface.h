#pragma once

#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanInstance;
    class Window;

    class VulkanSurface
    {
    public:
        VulkanSurface() = default;
        ~VulkanSurface() = default;

        VulkanSurface(const VulkanSurface&) = delete;
        VulkanSurface& operator=(const VulkanSurface&) = delete;
        VulkanSurface(VulkanSurface&&) = delete;
        VulkanSurface& operator=(VulkanSurface&&) = delete;

        void Initialize(VulkanInstance& instance, Window& window);
        void Shutdown(VulkanInstance& instance);

        VkSurfaceKHR GetSurface() const { return m_SurfaceHandle; }

    private:
        void CreateSurface(VulkanInstance& instance, Window& window);
        void DestroySurface(VulkanInstance& instance);

    private:
        VkSurfaceKHR m_SurfaceHandle = VK_NULL_HANDLE;
    };
}