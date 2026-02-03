#include "Trinity/Renderer/Vulkan/VulkanSurface.h"

#include "Trinity/Platform/Window.h"
#include "Trinity/Renderer/Vulkan/VulkanInstance.h"
#include "Trinity/Utilities/Utilities.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan_win32.h>
#endif

#include <cstdlib>
#include <cstring>

namespace Trinity
{
    void VulkanSurface::Initialize(VulkanInstance& instance, Window& window)
    {
        CreateSurface(instance, window);
    }

    void VulkanSurface::Shutdown(VulkanInstance& instance)
    {
        DestroySurface(instance);
    }

    void VulkanSurface::CreateSurface(VulkanInstance& instance, Window& window)
    {
        TR_CORE_TRACE("Creating window surface");

        if (m_SurfaceHandle != VK_NULL_HANDLE)
        {
            return;
        }

#if defined(_WIN32)
        // Sanity check: the instance must have enabled win32 surface.
        bool l_HasWin32Surface = false;
        for (const char* it_Ext : instance.GetEnabledExtensions())
        {
            if (std::strcmp(it_Ext, "VK_KHR_win32_surface") == 0)
            {
                l_HasWin32Surface = true;

                break;
            }
        }

        if (!l_HasWin32Surface)
        {
            TR_CORE_CRITICAL("VK_KHR_win32_surface extension not enabled. Surface creation cannot proceed.");

            std::abort();
        }

        const NativeWindowHandle l_NativeWindow = window.GetNativeHandle();
        if (l_NativeWindow.Type != NativeWindowType::Win32 || l_NativeWindow.Window == nullptr || l_NativeWindow.Instance == nullptr)
        {
            TR_CORE_CRITICAL("VulkanSurface::CreateSurface requires a valid Win32 window handle");

            std::abort();
        }

        VkWin32SurfaceCreateInfoKHR l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        l_CreateInfo.hwnd = reinterpret_cast<HWND>(l_NativeWindow.Window);
        l_CreateInfo.hinstance = reinterpret_cast<HINSTANCE>(l_NativeWindow.Instance);

        Utilities::VulkanUtilities::VKCheck(vkCreateWin32SurfaceKHR(instance.GetInstance(), &l_CreateInfo, instance.GetAllocator(), &m_SurfaceHandle), "Failed vkCreateWin32SurfaceKHR");

        TR_CORE_TRACE("Window surface created");
#else
        (void)instance;
        (void)window;

        TR_CORE_CRITICAL("VulkanSurface::CreateSurface called on unsupported platform.");

        std::abort();
#endif
    }

    void VulkanSurface::DestroySurface(VulkanInstance& instance)
    {
        TR_CORE_TRACE("Destroying window surface");

        if (m_SurfaceHandle == VK_NULL_HANDLE)
        {
            return;
        }

        vkDestroySurfaceKHR(instance.GetInstance(), m_SurfaceHandle, instance.GetAllocator());
        m_SurfaceHandle = VK_NULL_HANDLE;

        TR_CORE_TRACE("Window surface destroyed");
    }
}