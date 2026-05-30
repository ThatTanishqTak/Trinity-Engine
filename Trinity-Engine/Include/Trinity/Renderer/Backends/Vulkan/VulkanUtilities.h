#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>

namespace Trinity
{
    namespace VulkanUtilities
    {
        struct VmaAllocationParameters
        {
            VmaMemoryUsage Usage = VMA_MEMORY_USAGE_AUTO;
            VmaAllocationCreateFlags Flags = 0;
        };

        VkFormat ToVkFormat(Format format);
        VkBufferUsageFlags ToVkBufferUsage(BufferUsage usage);
        VkImageUsageFlags ToVkImageUsage(TextureUsage usage);
        VmaAllocationParameters ToVmaAllocation(MemoryUsage memory);
        VkPrimitiveTopology ToVkTopology(PrimitiveTopology topology);
        VkCullModeFlags ToVkCullMode(CullMode mode);
        VkFrontFace ToVkFrontFace(FrontFace face);
        VkCompareOp ToVkCompareOp(CompareOp op);
        VkShaderStageFlags ToVkShaderStages(ShaderStage stages);
    }
}