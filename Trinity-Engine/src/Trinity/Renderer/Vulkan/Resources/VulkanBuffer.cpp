#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"

#include "Trinity/Renderer/Vulkan/VulkanUploadQueue.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

#include <cstring>

namespace Trinity
{
    VulkanBuffer::VulkanBuffer(VkDevice device, VmaAllocator allocator, const BufferSpecification& specification, VulkanUploadQueue* uploadQueue) : m_Device(device), m_Allocator(allocator), m_UploadQueue(uploadQueue)
    {
        m_Specification = specification;

        VkBufferCreateInfo l_BufferInfo{};
        l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        l_BufferInfo.size = specification.Size;
        l_BufferInfo.usage = VulkanUtilities::ToVkBufferUsage(specification.Usage);
        if (specification.MemoryType == BufferMemoryType::GPU && m_UploadQueue != nullptr)
        {
            l_BufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        l_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo l_AllocateInfo{};
        switch (specification.MemoryType)
        {
            case BufferMemoryType::GPU:
            {
                l_AllocateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

                break;
            }
            case BufferMemoryType::CPUToGPU:
            {
                l_AllocateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
                l_AllocateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

                break;
            }
            case BufferMemoryType::GPUToCPU:
            {
                l_AllocateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
                l_AllocateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

                break;
            }
            default:
            {
                break;
            }
        }

        VulkanUtilities::VKCheck(vmaCreateBuffer(m_Allocator, &l_BufferInfo, &l_AllocateInfo, &m_Buffer, &m_Allocation, nullptr), "Failed vmaCreateBuffer");

        if (specification.MemoryType == BufferMemoryType::CPUToGPU || specification.MemoryType == BufferMemoryType::GPUToCPU)
        {
            vmaMapMemory(m_Allocator, m_Allocation, &m_MappedData);
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
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
        if (data == nullptr || size == 0)
        {
            TR_CORE_WARN("VulkanBuffer::SetData [{}] called with empty data (size={}, data={})", m_Specification.DebugName, size, data != nullptr);
            return;
        }

        if (offset + size > m_Specification.Size)
        {
            TR_CORE_ERROR("VulkanBuffer::SetData [{}] write out of range (offset={}, size={}, bufferSize={})", m_Specification.DebugName, offset, size, m_Specification.Size);
            return;
        }

        if (m_Specification.MemoryType == BufferMemoryType::CPUToGPU || m_Specification.MemoryType == BufferMemoryType::GPUToCPU)
        {
            if (m_MappedData != nullptr)
            {
                std::memcpy(static_cast<uint8_t*>(m_MappedData) + offset, data, static_cast<size_t>(size));
                vmaFlushAllocation(m_Allocator, m_Allocation, offset, size);
            }
            else
            {
                void* l_Mapped = nullptr;
                VulkanUtilities::VKCheck(vmaMapMemory(m_Allocator, m_Allocation, &l_Mapped), "Failed vmaMapMemory in SetData");
                std::memcpy(static_cast<uint8_t*>(l_Mapped) + offset, data, static_cast<size_t>(size));
                vmaFlushAllocation(m_Allocator, m_Allocation, offset, size);
                vmaUnmapMemory(m_Allocator, m_Allocation);
            }

            return;
        }

        if (m_UploadQueue == nullptr)
        {
            TR_CORE_ERROR("VulkanBuffer::SetData [{}] on GPU-only buffer requires an upload queue", m_Specification.DebugName);
            return;
        }

        TR_CORE_TRACE("VulkanBuffer::SetData [{}] routing {} bytes through upload queue (offset={})", m_Specification.DebugName, size, offset);
        m_UploadQueue->EnqueueBufferUpload(m_Buffer, offset, data, size);
    }

    void* VulkanBuffer::Map()
    {
        if (m_MappedData != nullptr)
        {
            return m_MappedData;
        }

        if (m_Specification.MemoryType == BufferMemoryType::GPU)
        {
            TR_CORE_ERROR("VulkanBuffer::Map [{}] called on GPU-only buffer; map is not supported, use SetData via upload queue", m_Specification.DebugName);
            return nullptr;
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