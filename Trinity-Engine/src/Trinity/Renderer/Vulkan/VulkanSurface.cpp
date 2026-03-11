#include "Trinity/Renderer/Vulkan/VulkanSurface.h"

#include "Trinity/Utilities/Log.h"
#include <SDL3/SDL_vulkan.h>

#include <cstdlib>

namespace Trinity
{
    VkSurfaceKHR CreateVulkanSurface(VkInstance instance, VkAllocationCallbacks* allocator, const NativeWindowHandle& nativeWindowHandle)
    {
        if (instance == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("CreateVulkanSurface called with null VkInstance");

            std::abort();
        }

        if (nativeWindowHandle.Window == nullptr)
        {
            TR_CORE_CRITICAL("CreateVulkanSurface called with null SDL_Window");

            std::abort();
        }

        VkSurfaceKHR l_Surface = VK_NULL_HANDLE;
        if (!SDL_Vulkan_CreateSurface(nativeWindowHandle.Window, instance, allocator, &l_Surface))
        {
            TR_CORE_CRITICAL("SDL_Vulkan_CreateSurface failed: {}", SDL_GetError());

            std::abort();
        }

        return l_Surface;
    }
}