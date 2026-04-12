#pragma once

#include "Trinity/Renderer/Resources/Buffer.h"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanBuffer final : public Buffer
    {
    public:
        VulkanBuffer(VkDevice device, VmaAllocator allocator, const BufferSpecification& specification);
        ~VulkanBuffer() override;

        void SetData(const void* data, uint64_t size, uint64_t offset = 0) override;
        void* Map() override;
        void Unmap() override;
        uint64_t GetSize() const override { return m_Specification.Size; }

        VkBuffer GetBuffer() const { return m_Buffer; }
 
    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        void* m_MappedData = nullptr;
    };
}