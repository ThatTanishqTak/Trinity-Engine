#include "Trinity/Renderer/Vulkan/VulkanSurface.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>

#if defined(_WIN32)
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

namespace Trinity
{
    VkSurfaceKHR CreateVulkanSurface(VkInstance instance, VkAllocationCallbacks* allocator, const NativeWindowHandle& nativeWindowHandle)
    {
        if (instance == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("CreateVulkanSurface called with null VkInstance");

            std::abort();
        }

        switch (nativeWindowHandle.WindowType)
        {
            case NativeWindowHandle::Type::Win32:
            {
#if defined(_WIN32)
                if (!nativeWindowHandle.Handle1)
                {
                    TR_CORE_CRITICAL("CreateVulkanSurface Win32 path received null HWND");

                    std::abort();
                }

                VkWin32SurfaceCreateInfoKHR l_SurfaceCreateInfo{};
                l_SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                l_SurfaceCreateInfo.hwnd = reinterpret_cast<HWND>(nativeWindowHandle.Handle1);
                l_SurfaceCreateInfo.hinstance = reinterpret_cast<HINSTANCE>(nativeWindowHandle.Handle2);

                VkSurfaceKHR l_Surface = VK_NULL_HANDLE;
                Utilities::VulkanUtilities::VKCheck(vkCreateWin32SurfaceKHR(instance, &l_SurfaceCreateInfo, allocator, &l_Surface), "Failed vkCreateWin32SurfaceKHR");

                return l_Surface;
#else
                TR_CORE_CRITICAL("Native handle type Win32 requested, but this build target does not support Win32 surface creation");

                std::abort();
#endif
            }
            case NativeWindowHandle::Type::Xcb:
            case NativeWindowHandle::Type::Wayland:
            case NativeWindowHandle::Type::Cocoa:
            case NativeWindowHandle::Type::Unknown:
            default:
            {
                TR_CORE_CRITICAL("CreateVulkanSurface does not support native window type {} in this build", static_cast<int>(nativeWindowHandle.WindowType));

                std::abort();
            }
        }
    }
}