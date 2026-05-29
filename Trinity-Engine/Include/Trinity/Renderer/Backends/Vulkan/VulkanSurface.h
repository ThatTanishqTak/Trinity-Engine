#pragma once

#include <vulkan/vulkan.h>

#include <Trinity/Platform/PlatformTypes.h>

namespace Trinity
{
    class VulkanSurface
    {
    public:
        VulkanSurface() = default;
        ~VulkanSurface();

        VulkanSurface(const VulkanSurface&) = delete;
        VulkanSurface& operator=(const VulkanSurface&) = delete;

        bool Initialize(VkInstance instance, const NativeWindowHandle& handle);
        void Shutdown();

        VkSurfaceKHR GetHandle() const { return m_Surface; }

    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    };
}