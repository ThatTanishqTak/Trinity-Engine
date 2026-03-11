#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"

#include "Trinity/Renderer/Vulkan/VulkanUploadContext.h"

#include "Trinity/Utilities/Log.h"

#include <cstring>
#include <cstdlib>
#include <utility>

namespace Trinity
{
	VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
	{
		*this = std::move(other);
	}

	VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		Destroy();

		m_VmaAllocator = other.m_VmaAllocator;
		m_Buffer = other.m_Buffer;
		m_Allocation = other.m_Allocation;
		m_Size = other.m_Size;
		m_MemoryUsage = other.m_MemoryUsage;
		m_Mapped = other.m_Mapped;
		m_UploadContext = other.m_UploadContext;

		other.m_VmaAllocator = nullptr;
		other.m_Buffer = VK_NULL_HANDLE;
		other.m_Allocation = VK_NULL_HANDLE;
		other.m_Size = 0;
		other.m_Mapped = nullptr;
		other.m_UploadContext = nullptr;

		return *this;
	}

	void VulkanBuffer::Create(VulkanAllocator& allocator, VkDeviceSize size, VkBufferUsageFlags usage, BufferMemoryUsage memoryUsage,
		VulkanUploadContext* uploadContext)
	{
		if (size == 0)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Create size=0");
			std::abort();
		}

		if (memoryUsage == BufferMemoryUsage::GPUOnly && uploadContext == nullptr)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Create - GPUOnly buffer requires a valid VulkanUploadContext");
			std::abort();
		}

		Destroy();

		m_VmaAllocator = &allocator;
		m_Size = size;
		m_MemoryUsage = memoryUsage;
		m_UploadContext = uploadContext;

		VkBufferUsageFlags l_UsageFlags = usage;
		VmaAllocationCreateFlags l_AllocFlags = 0;

		if (memoryUsage == BufferMemoryUsage::CPUToGPU)
		{
			l_AllocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else // GPUOnly
		{
			l_UsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}

		CreateRawBuffer(size, l_UsageFlags, l_AllocFlags);
	}

	void VulkanBuffer::Destroy()
	{
		if (m_VmaAllocator == nullptr)
		{
			return;
		}

		Unmap();

		if (m_Buffer != VK_NULL_HANDLE)
		{
			m_VmaAllocator->DestroyBuffer(m_Buffer, m_Allocation);
			m_Buffer = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;
		}

		m_Size = 0;
		m_VmaAllocator = nullptr;
		m_Mapped = nullptr;
		m_MemoryUsage = BufferMemoryUsage::GPUOnly;
		m_UploadContext = nullptr;
	}

	VulkanBuffer VulkanBuffer::CreateStaging(const VulkanBuffer& source, VkDeviceSize size)
	{
		VulkanBuffer l_Staging{};
		l_Staging.m_VmaAllocator = source.m_VmaAllocator;
		l_Staging.m_Size = size;
		l_Staging.m_MemoryUsage = BufferMemoryUsage::CPUToGPU;
		l_Staging.CreateRawBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		return l_Staging;
	}

	void VulkanBuffer::SetData(const void* data, VkDeviceSize size, VkDeviceSize offset)
	{
		if (!data)
		{
			TR_CORE_CRITICAL("VulkanBuffer::SetData - null data pointer");
			std::abort();
		}

		if (size == 0)
		{
			return;
		}

		if (offset + size > m_Size)
		{
			TR_CORE_CRITICAL("VulkanBuffer::SetData - write range [offset={}, size={}] exceeds buffer size {}", offset, size, m_Size);
			std::abort();
		}

		if (m_MemoryUsage == BufferMemoryUsage::CPUToGPU)
		{
			void* l_Dst = Map(offset);
			std::memcpy(l_Dst, data, (size_t)size);
			Unmap();

			return;
		}

		// GPUOnly path: write into a temporary staging buffer then copy.
		if (!m_UploadContext)
		{
			TR_CORE_CRITICAL("VulkanBuffer::SetData - GPUOnly buffer has no upload context");
			std::abort();
		}

		VulkanBuffer l_Staging = CreateStaging(*this, size);

		void* l_Map = l_Staging.Map(0);
		std::memcpy(l_Map, data, (size_t)size);
		l_Staging.Unmap();

		m_UploadContext->CopyBuffer(l_Staging.m_Buffer, m_Buffer, size, 0, offset);

		l_Staging.Destroy();
	}

	void* VulkanBuffer::Map(VkDeviceSize offset)
	{
		if (m_MemoryUsage != BufferMemoryUsage::CPUToGPU)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Map - cannot map a GPUOnly buffer");
			std::abort();
		}

		if (!m_Mapped)
		{
			m_Mapped = m_VmaAllocator->MapMemory(m_Allocation);
		}

		return static_cast<uint8_t*>(m_Mapped) + offset;
	}

	void VulkanBuffer::Unmap()
	{
		if (m_Mapped)
		{
			m_VmaAllocator->UnmapMemory(m_Allocation);
			m_Mapped = nullptr;
		}
	}

	void VulkanBuffer::CreateRawBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags allocationFlags)
	{
		m_VmaAllocator->CreateBuffer(size, usage, VMA_MEMORY_USAGE_AUTO, allocationFlags, m_Buffer, m_Allocation);
	}

	VulkanVertexBuffer::VulkanVertexBuffer(VulkanAllocator& allocator, VulkanUploadContext& uploadContext, uint64_t size, uint32_t stride,
		BufferMemoryUsage memoryUsage, const void* initialData) : m_Stride(stride)
	{
		if (stride == 0)
		{
			TR_CORE_CRITICAL("VulkanVertexBuffer: stride=0");
			std::abort();
		}

		m_Buffer.Create(allocator, (VkDeviceSize)size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, memoryUsage,
			memoryUsage == BufferMemoryUsage::GPUOnly ? &uploadContext : nullptr);

		if (initialData && size > 0)
		{
			m_Buffer.SetData(initialData, (VkDeviceSize)size, 0);
		}
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		m_Buffer.Destroy();
	}

	void VulkanVertexBuffer::SetData(const void* data, uint64_t size, uint64_t offset)
	{
		m_Buffer.SetData(data, (VkDeviceSize)size, (VkDeviceSize)offset);
	}

	VulkanIndexBuffer::VulkanIndexBuffer(VulkanAllocator& allocator, VulkanUploadContext& uploadContext, uint64_t size, uint32_t indexCount,
		IndexType indexType, BufferMemoryUsage memoryUsage, const void* initialData) : m_IndexCount(indexCount), m_IndexType(indexType)
	{
		m_Buffer.Create(allocator, (VkDeviceSize)size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, memoryUsage,
			memoryUsage == BufferMemoryUsage::GPUOnly ? &uploadContext : nullptr);

		if (initialData && size > 0)
		{
			m_Buffer.SetData(initialData, (VkDeviceSize)size, 0);
		}
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		m_Buffer.Destroy();
	}

	void VulkanIndexBuffer::SetData(const void* data, uint64_t size, uint64_t offset)
	{
		m_Buffer.SetData(data, (VkDeviceSize)size, (VkDeviceSize)offset);
	}

	VulkanUniformBuffer::VulkanUniformBuffer(VulkanAllocator& allocator, uint64_t size)
	{
		if (size == 0)
		{
			TR_CORE_CRITICAL("VulkanUniformBuffer: size = 0");

			std::abort();
		}

		const VkDeviceSize l_Alignment = allocator.GetMinUniformBufferOffsetAlignment();
		const VkDeviceSize l_AlignedSize = (size + l_Alignment - 1) & ~(l_Alignment - 1);

		m_Buffer.Create(allocator, l_AlignedSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, BufferMemoryUsage::CPUToGPU, /* uploadContext */ nullptr);

		m_MappedPtr = m_Buffer.Map(0);
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		if (m_MappedPtr)
		{
			m_Buffer.Unmap();
			m_MappedPtr = nullptr;
		}

		m_Buffer.Destroy();
	}

	VulkanUniformBuffer::VulkanUniformBuffer(VulkanUniformBuffer&& other) noexcept
	{
		*this = std::move(other);
	}

	VulkanUniformBuffer& VulkanUniformBuffer::operator=(VulkanUniformBuffer&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		// Clean up our own resources first.
		if (m_MappedPtr)
		{
			m_Buffer.Unmap();
			m_MappedPtr = nullptr;
		}

		m_Buffer.Destroy();

		m_Buffer = std::move(other.m_Buffer);
		m_MappedPtr = other.m_MappedPtr;

		other.m_MappedPtr = nullptr;

		return *this;
	}

	void VulkanUniformBuffer::SetData(const void* data, uint64_t size, uint64_t offset)
	{
		if (!data)
		{
			TR_CORE_CRITICAL("VulkanUniformBuffer::SetData - null data pointer");
			std::abort();
		}

		if (offset + size > (uint64_t)m_Buffer.GetSize())
		{
			TR_CORE_CRITICAL("VulkanUniformBuffer::SetData write range [offset = {}, size = {}] exceeds buffer size {}", offset, size, m_Buffer.GetSize());

			std::abort();
		}

		std::memcpy(static_cast<uint8_t*>(m_MappedPtr) + offset, data, (size_t)size);
	}

	VkDescriptorBufferInfo VulkanUniformBuffer::GetDescriptorBufferInfo(VkDeviceSize offset, VkDeviceSize range) const
	{
		VkDescriptorBufferInfo l_Info{};
		l_Info.buffer = m_Buffer.GetBuffer();
		l_Info.offset = offset;
		l_Info.range = range;

		return l_Info;
	}
}