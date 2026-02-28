#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanUploadContext.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

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

		m_Allocator = other.m_Allocator;
		m_PhysicalDevice = other.m_PhysicalDevice;
		m_Device = other.m_Device;
		m_Buffer = other.m_Buffer;
		m_Memory = other.m_Memory;
		m_Size = other.m_Size;
		m_MemoryUsage = other.m_MemoryUsage;
		m_Mapped = other.m_Mapped;
		m_UploadContext = other.m_UploadContext;

		other.m_Allocator = nullptr;
		other.m_PhysicalDevice = VK_NULL_HANDLE;
		other.m_Device = VK_NULL_HANDLE;
		other.m_Buffer = VK_NULL_HANDLE;
		other.m_Memory = VK_NULL_HANDLE;
		other.m_Size = 0;
		other.m_Mapped = nullptr;
		other.m_UploadContext = nullptr;

		return *this;
	}

	void VulkanBuffer::Create(const VulkanContext& context, const VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, BufferMemoryUsage memoryUsage,
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

		m_Allocator = context.GetAllocator();
		m_PhysicalDevice = device.GetPhysicalDevice();
		m_Device = device.GetDevice();
		m_Size = size;
		m_MemoryUsage = memoryUsage;
		m_UploadContext = uploadContext;

		VkMemoryPropertyFlags l_MemProps = 0;
		VkBufferUsageFlags    l_UsageFlags = usage;

		if (memoryUsage == BufferMemoryUsage::CPUToGPU)
		{
			l_MemProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
		else // GPUOnly
		{
			l_MemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			l_UsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}

		CreateRawBuffer(size, l_UsageFlags, l_MemProps);
	}

	void VulkanBuffer::Destroy()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		Unmap();

		if (m_Buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(m_Device, m_Buffer, m_Allocator);
			m_Buffer = VK_NULL_HANDLE;
		}

		if (m_Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_Device, m_Memory, m_Allocator);
			m_Memory = VK_NULL_HANDLE;
		}

		m_Size = 0;
		m_Allocator = nullptr;
		m_PhysicalDevice = VK_NULL_HANDLE;
		m_Device = VK_NULL_HANDLE;
		m_Mapped = nullptr;
		m_MemoryUsage = BufferMemoryUsage::GPUOnly;
		m_UploadContext = nullptr;
	}

	VulkanBuffer VulkanBuffer::CreateStaging(const VulkanBuffer& source, VkDeviceSize size)
	{
		VulkanBuffer l_Staging{};
		l_Staging.m_Allocator = source.m_Allocator;
		l_Staging.m_PhysicalDevice = source.m_PhysicalDevice;
		l_Staging.m_Device = source.m_Device;
		l_Staging.m_Size = size;
		l_Staging.m_MemoryUsage = BufferMemoryUsage::CPUToGPU;
		l_Staging.CreateRawBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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
			Utilities::VulkanUtilities::VKCheck(vkMapMemory(m_Device, m_Memory, 0, VK_WHOLE_SIZE, 0, &m_Mapped), "VulkanBuffer: vkMapMemory failed");
		}

		return static_cast<uint8_t*>(m_Mapped) + offset;
	}

	void VulkanBuffer::Unmap()
	{
		if (m_Mapped)
		{
			vkUnmapMemory(m_Device, m_Memory);
			m_Mapped = nullptr;
		}
	}

	void VulkanBuffer::CreateRawBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		VkBufferCreateInfo l_BufferInfo{};
		l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		l_BufferInfo.size = size;
		l_BufferInfo.usage = usage;
		l_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		Utilities::VulkanUtilities::VKCheck(vkCreateBuffer(m_Device, &l_BufferInfo, m_Allocator, &m_Buffer), "VulkanBuffer: vkCreateBuffer failed");

		VkMemoryRequirements l_MemReqs{};
		vkGetBufferMemoryRequirements(m_Device, m_Buffer, &l_MemReqs);

		VkMemoryAllocateInfo l_AllocInfo{};
		l_AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		l_AllocInfo.allocationSize = l_MemReqs.size;
		l_AllocInfo.memoryTypeIndex = Utilities::VulkanUtilities::FindMemoryType(m_PhysicalDevice, l_MemReqs.memoryTypeBits, properties);
		Utilities::VulkanUtilities::VKCheck(vkAllocateMemory(m_Device, &l_AllocInfo, m_Allocator, &m_Memory), "VulkanBuffer: vkAllocateMemory failed");

		Utilities::VulkanUtilities::VKCheck(vkBindBufferMemory(m_Device, m_Buffer, m_Memory, 0), "VulkanBuffer: vkBindBufferMemory failed");
	}

	VulkanVertexBuffer::VulkanVertexBuffer(const VulkanContext& context, const VulkanDevice& device, VulkanUploadContext& uploadContext, uint64_t size, uint32_t stride,
		BufferMemoryUsage memoryUsage, const void* initialData) : m_Stride(stride)
	{
		if (stride == 0)
		{
			TR_CORE_CRITICAL("VulkanVertexBuffer: stride=0");
			std::abort();
		}

		m_Buffer.Create(context, device, (VkDeviceSize)size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, memoryUsage,
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

	VulkanIndexBuffer::VulkanIndexBuffer(const VulkanContext& context, const VulkanDevice& device, VulkanUploadContext& uploadContext, uint64_t size, uint32_t indexCount,
		IndexType indexType, BufferMemoryUsage memoryUsage, const void* initialData) : m_IndexCount(indexCount), m_IndexType(indexType)
	{
		m_Buffer.Create(context, device, (VkDeviceSize)size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, memoryUsage, memoryUsage == BufferMemoryUsage::GPUOnly ? &uploadContext : nullptr);

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
}