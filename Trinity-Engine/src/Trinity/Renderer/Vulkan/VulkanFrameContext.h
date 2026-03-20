#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Trinity
{
    struct VulkanFrameContext
    {
        VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
        uint32_t FrameIndex = 0;
        uint32_t ImageIndex = 0;
        uint32_t ViewportWidth = 0;
        uint32_t ViewportHeight = 0;
    };
}