#pragma once

#include "Trinity/Platform/Window.h"

#include <vulkan/vulkan.h>

namespace Trinity
{
    VkSurfaceKHR CreateVulkanSurface(VkInstance instance, VkAllocationCallbacks* allocator, const NativeWindowHandle& nativeWindowHandle);
}