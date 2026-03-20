#pragma once

#include "Trinity/Renderer/Vulkan/VulkanAllocator.h"
#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"
#include "Trinity/Renderer/Vulkan/VulkanUploadContext.h"
#include "Trinity/Renderer/Vulkan/VulkanResourceStateTracker.h"
#include "Trinity/Renderer/ShaderLibrary.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Trinity
{
    struct VulkanPassContext
    {
        VkDevice Device = VK_NULL_HANDLE;
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        VkAllocationCallbacks* HostAllocator = nullptr;
        VulkanAllocator* Allocator = nullptr;
        VulkanUploadContext* UploadContext = nullptr;
        VulkanResourceStateTracker* ResourceStateTracker = nullptr;
        ShaderLibrary* Shaders = nullptr;
        VkQueue GraphicsQueue = VK_NULL_HANDLE;
        uint32_t GraphicsQueueFamilyIndex = 0;
        VkFormat SwapchainFormat = VK_FORMAT_UNDEFINED;
        VkFormat DepthFormat = VK_FORMAT_UNDEFINED;
        uint32_t FramesInFlight = 0;
        VulkanSwapchain::SceneColorPolicy ColorPolicy{};
    };
}