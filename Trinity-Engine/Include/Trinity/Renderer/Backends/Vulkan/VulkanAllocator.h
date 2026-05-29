#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Trinity
{
    class VulkanAllocator
    {
    public:
        VulkanAllocator() = default;
        ~VulkanAllocator();

        VulkanAllocator(const VulkanAllocator&) = delete;
        VulkanAllocator& operator=(const VulkanAllocator&) = delete;

        bool Initialize(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
        void Shutdown();

        VmaAllocator GetHandle() const { return m_Allocator; }

        void LogStats() const;

    private:
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
    };
}