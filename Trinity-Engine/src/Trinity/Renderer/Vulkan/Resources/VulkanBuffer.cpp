#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

#include <cstring>

namespace Trinity
{
    VulkanBuffer::VulkanBuffer(VkDevice device, VmaAllocator allocator, const BufferSpecification& specification) : m_Device(device), m_Allocator(allocator)
    {
        m_Specification = specification;

        VkBufferCreateInfo l_BufferInfo{};
        l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        l_BufferInfo.size = specification.Size;
        l_BufferInfo.usage = VulkanUtilities::ToVkBufferUsage(specification.Usage);
        l_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo l_AllocInfo{};
        switch (specification.MemoryType)
        {
            case BufferMemoryType::GPU:
                l_AllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
                break;
            case BufferMemoryType::CPUToGPU:
                l_AllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
                l_AllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;
            case BufferMemoryType::GPUToCPU:
                l_AllocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
                l_AllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;
        }

        VulkanUtilities::VKCheck(vmaCreateBuffer(m_Allocator, &l_BufferInfo, &l_AllocInfo, &m_Buffer, &m_Allocation, nullptr), "Failed vmaCreateBuffer");

        if (specification.MemoryType == BufferMemoryType::CPUToGPU || specification.MemoryType == BufferMemoryType::GPUToCPU)
        {
            vmaMapMemory(m_Allocator, m_Allocation, &m_MappedData);
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        Renderer::WaitIdle();

        if (m_MappedData != nullptr)
        {
            vmaUnmapMemory(m_Allocator, m_Allocation);
            m_MappedData = nullptr;
        }

        if (m_Buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_Allocator, m_Buffer, m_Allocation);
        }
    }

    void VulkanBuffer::SetData(const void* data, uint64_t size, uint64_t offset)
    {
        if (m_MappedData != nullptr)
        {
            std::memcpy(static_cast<uint8_t*>(m_MappedData) + offset, data, size);
            vmaFlushAllocation(m_Allocator, m_Allocation, offset, size);
        }
        else
        {
            void* l_Mapped = nullptr;

            vmaMapMemory(m_Allocator, m_Allocation, &l_Mapped);
            std::memcpy(static_cast<uint8_t*>(l_Mapped) + offset, data, size);
            vmaFlushAllocation(m_Allocator, m_Allocation, offset, size);
            vmaUnmapMemory(m_Allocator, m_Allocation);
        }
    }

    void* VulkanBuffer::Map()
    {
        if (m_MappedData != nullptr)
        {
            return m_MappedData;
        }

        void* l_Data = nullptr;
        VulkanUtilities::VKCheck(vmaMapMemory(m_Allocator, m_Allocation, &l_Data), "Failed vmaMapMemory");
        m_MappedData = l_Data;

        return l_Data;
    }

    void VulkanBuffer::Unmap()
    {
        if (m_MappedData != nullptr && m_Specification.MemoryType == BufferMemoryType::GPU)
        {
            vmaUnmapMemory(m_Allocator, m_Allocation);
            m_MappedData = nullptr;
        }
    }
}