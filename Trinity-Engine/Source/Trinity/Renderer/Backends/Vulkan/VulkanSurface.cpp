#include <Trinity/Renderer/Backends/Vulkan/VulkanSurface.h>

#include <Trinity/Core/Log.h>

#if defined(TRINITY_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined(TRINITY_PLATFORM_LINUX)
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_wayland.h>
#elif defined(TRINITY_PLATFORM_MACOS)
#include <vulkan/vulkan_metal.h>
#endif

namespace Trinity
{
    VulkanSurface::~VulkanSurface()
    {
        Shutdown();
    }

    bool VulkanSurface::Initialize(VkInstance instance, const NativeWindowHandle& handle)
    {
        if (instance == VK_NULL_HANDLE)
        {
            ("VulkanSurface: instance is null");
            return false;
        }

        if (!handle.IsValid())
        {
            ("VulkanSurface: native handle is invalid");
            return false;
        }

        m_Instance = instance;

        VkResult l_Result = VK_ERROR_UNKNOWN;

#if defined(TRINITY_PLATFORM_WINDOWS)
        if (handle.Type != NativeHandleType::Win32)
        {
            ("VulkanSurface: expected Win32 handle on Windows");
            return false;
        }

        VkWin32SurfaceCreateInfoKHR l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        l_CreateInfo.hinstance = GetModuleHandle(nullptr);
        l_CreateInfo.hwnd = static_cast<HWND>(handle.Handle);

        l_Result = vkCreateWin32SurfaceKHR(m_Instance, &l_CreateInfo, nullptr, &m_Surface);
#elif defined(TRINITY_PLATFORM_LINUX)
        if (handle.Type == NativeHandleType::Wayland)
        {
            VkWaylandSurfaceCreateInfoKHR l_CreateInfo{};
            l_CreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
            l_CreateInfo.display = static_cast<wl_display*>(handle.Display);
            l_CreateInfo.surface = static_cast<wl_surface*>(handle.Handle);

            l_Result = vkCreateWaylandSurfaceKHR(m_Instance, &l_CreateInfo, nullptr, &m_Surface);
        }
        else if (handle.Type == NativeHandleType::Xlib)
        {
            VkXlibSurfaceCreateInfoKHR l_CreateInfo{};
            l_CreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            l_CreateInfo.dpy = static_cast<Display*>(handle.Display);
            l_CreateInfo.window = reinterpret_cast<Window>(handle.Handle);

            l_Result = vkCreateXlibSurfaceKHR(m_Instance, &l_CreateInfo, nullptr, &m_Surface);
        }
        else
        {
            ("VulkanSurface: unsupported handle type on Linux");
            return false;
        }
#elif defined(TRINITY_PLATFORM_MACOS)
        if (handle.Type != NativeHandleType::Cocoa)
        {
            ("VulkanSurface: expected Cocoa handle on macOS");
            return false;
        }

        VkMetalSurfaceCreateInfoEXT l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        l_CreateInfo.pLayer = handle.Handle;

        l_Result = vkCreateMetalSurfaceEXT(m_Instance, &l_CreateInfo, nullptr, &m_Surface);
#else
        ("VulkanSurface: no platform-specific surface creation available");
        return false;
#endif

        if (l_Result != VK_SUCCESS)
        {
            ("VulkanSurface: surface creation failed ({})", static_cast<int>(l_Result));
            return false;
        }

        ("VulkanSurface: created");
        return true;
    }

    void VulkanSurface::Shutdown()
    {
        if (m_Surface != VK_NULL_HANDLE && m_Instance != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }
    }
}