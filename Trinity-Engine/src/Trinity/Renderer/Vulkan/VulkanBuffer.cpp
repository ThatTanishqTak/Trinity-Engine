#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstring>
#include <cstdlib>
#include <utility>

namespace Trinity
{
	VulkanBuffer::~VulkanBuffer()
	{
		Destroy();
	}

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
		m_Queue = other.m_Queue;
		m_QueueFamilyIndex = other.m_QueueFamilyIndex;

		m_Buffer = other.m_Buffer;
		m_Memory = other.m_Memory;
		m_Size = other.m_Size;
		m_MemoryUsage = other.m_MemoryUsage;
		m_Mapped = other.m_Mapped;

		other.m_Allocator = nullptr;
		other.m_PhysicalDevice = VK_NULL_HANDLE;
		other.m_Device = VK_NULL_HANDLE;
		other.m_Queue = VK_NULL_HANDLE;
		other.m_QueueFamilyIndex = 0;
		other.m_Buffer = VK_NULL_HANDLE;
		other.m_Memory = VK_NULL_HANDLE;
		other.m_Size = 0;
		other.m_Mapped = nullptr;

		return *this;
	}

	void VulkanBuffer::Create(const VulkanContext& context, const VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, BufferMemoryUsage memoryUsage)
	{
		if (size == 0)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Create size=0");

			std::abort();
		}

		Destroy();

		m_Allocator = context.GetAllocator();
		m_PhysicalDevice = device.GetPhysicalDevice();
		m_Device = device.GetDevice();
		m_Queue = device.GetGraphicsQueue();
		m_QueueFamilyIndex = device.GetGraphicsQueueFamilyIndex();
		m_Size = size;
		m_MemoryUsage = memoryUsage;

		VkMemoryPropertyFlags l_PhysicalDeviceMemoryProperties = 0;
		VkBufferUsageFlags l_BufferUsageFlags = usage;

		if (memoryUsage == BufferMemoryUsage::CPUToGPU)
		{
			l_PhysicalDeviceMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
		else
		{
			l_PhysicalDeviceMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			l_BufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}

		CreateRawBuffer(size, l_BufferUsageFlags, l_PhysicalDeviceMemoryProperties);
	}

	void VulkanBuffer::Destroy()
	{
		if (m_Device == VK_NULL_HANDLE)
			return;

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
		m_Queue = VK_NULL_HANDLE;
		m_QueueFamilyIndex = 0;
		m_Mapped = nullptr;
		m_MemoryUsage = BufferMemoryUsage::GPUOnly;
	}

	void VulkanBuffer::SetData(const void* data, VkDeviceSize size, VkDeviceSize offset)
	{
		if (!data)
		{
			TR_CORE_CRITICAL("VulkanBuffer::SetData null data");

			std::abort();
		}

		if (offset + size > m_Size)
		{
			TR_CORE_CRITICAL("VulkanBuffer::SetData OOB");

			std::abort();
		}

		if (m_MemoryUsage == BufferMemoryUsage::CPUToGPU)
		{
			void* l_DestinationBuffer = Map(offset, size);
			std::memcpy(l_DestinationBuffer, data, (size_t)size);
			Unmap();

			return;
		}

		// GPUOnly: stage and buffer
		VulkanBuffer l_StagingBuffers{};
		l_StagingBuffers.m_Allocator = m_Allocator;
		l_StagingBuffers.m_PhysicalDevice = m_PhysicalDevice;
		l_StagingBuffers.m_Device = m_Device;
		l_StagingBuffers.m_Queue = m_Queue;
		l_StagingBuffers.m_QueueFamilyIndex = m_QueueFamilyIndex;
		l_StagingBuffers.m_Size = size;
		l_StagingBuffers.m_MemoryUsage = BufferMemoryUsage::CPUToGPU;
		l_StagingBuffers.CreateRawBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* l_Map = l_StagingBuffers.Map(0, size);
		std::memcpy(l_Map, data, (size_t)size);
		l_StagingBuffers.Unmap();

		CopyBufferImmediate(l_StagingBuffers.m_Buffer, m_Buffer, size);
	}

	void* VulkanBuffer::Map(VkDeviceSize offset, VkDeviceSize size)
	{
		(void)size;

		if (m_MemoryUsage != BufferMemoryUsage::CPUToGPU)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Map on GPUOnly buffer");

			std::abort();
		}

		if (!m_Mapped)
		{
			Utilities::VulkanUtilities::VKCheck(vkMapMemory(m_Device, m_Memory, 0, VK_WHOLE_SIZE, 0, &m_Mapped), "Failed vkMapMemory");
		}

		return (uint8_t*)m_Mapped + offset;
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
		VkBufferCreateInfo l_BufferCreateInfo{};
		l_BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		l_BufferCreateInfo.size = size;
		l_BufferCreateInfo.usage = usage;
		l_BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		Utilities::VulkanUtilities::VKCheck(vkCreateBuffer(m_Device, &l_BufferCreateInfo, m_Allocator, &m_Buffer), "Failed vkCreateBuffer");

		VkMemoryRequirements l_MemoryRequirements{};
		vkGetBufferMemoryRequirements(m_Device, m_Buffer, &l_MemoryRequirements);

		VkMemoryAllocateInfo l_MemoryAllocateInfo{};
		l_MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		l_MemoryAllocateInfo.allocationSize = l_MemoryRequirements.size;
		l_MemoryAllocateInfo.memoryTypeIndex = FindMemoryType(l_MemoryRequirements.memoryTypeBits, properties);

		Utilities::VulkanUtilities::VKCheck(vkAllocateMemory(m_Device, &l_MemoryAllocateInfo, m_Allocator, &m_Memory), "Failed vkAllocateMemory");
		Utilities::VulkanUtilities::VKCheck(vkBindBufferMemory(m_Device, m_Buffer, m_Memory, 0), "Failed vkBindBufferMemory");
	}

	uint32_t VulkanBuffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
	{
		VkPhysicalDeviceMemoryProperties l_PhysicalDeviceMemoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &l_PhysicalDeviceMemoryProperties);

		for (uint32_t i = 0; i < l_PhysicalDeviceMemoryProperties.memoryTypeCount; ++i)
		{
			if ((typeFilter & (1u << i)) && (l_PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		TR_CORE_CRITICAL("Failed to find suitable Vulkan memory type");

		std::abort();
	}

	void VulkanBuffer::CopyBufferImmediate(VkBuffer sourceBuffer, VkBuffer l_DestinationBuffer, VkDeviceSize size)
	{
		VkCommandPool l_CommandPool = VK_NULL_HANDLE;
		VkCommandBuffer l_CommandBuffer = VK_NULL_HANDLE;
		VkFence l_Fence = VK_NULL_HANDLE;

		VkCommandPoolCreateInfo l_CommandPoolCreateInfo{};
		l_CommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		l_CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		l_CommandPoolCreateInfo.queueFamilyIndex = m_QueueFamilyIndex;

		Utilities::VulkanUtilities::VKCheck(vkCreateCommandPool(m_Device, &l_CommandPoolCreateInfo, m_Allocator, &l_CommandPool), "Failed vkCreateCommandPool");

		VkCommandBufferAllocateInfo l_CommandBufferAllocateInfo{};
		l_CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		l_CommandBufferAllocateInfo.commandPool = l_CommandPool;
		l_CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		l_CommandBufferAllocateInfo.commandBufferCount = 1;

		Utilities::VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_Device, &l_CommandBufferAllocateInfo, &l_CommandBuffer), "Failed vkAllocateCommandBuffers");

		VkCommandBufferBeginInfo l_CommandBufferBeginInfo{};
		l_CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		l_CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		Utilities::VulkanUtilities::VKCheck(vkBeginCommandBuffer(l_CommandBuffer, &l_CommandBufferBeginInfo), "Failed vkBeginCommandBuffer");

		VkBufferCopy l_BufferCopy{};
		l_BufferCopy.size = size;
		vkCmdCopyBuffer(l_CommandBuffer, sourceBuffer, l_DestinationBuffer, 1, &l_BufferCopy);

		Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(l_CommandBuffer), "Failed vkEndCommandBuffer");

		VkFenceCreateInfo l_FenceInfo{};
		l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		Utilities::VulkanUtilities::VKCheck(vkCreateFence(m_Device, &l_FenceInfo, m_Allocator, &l_Fence), "Failed vkCreateFence");

		VkSubmitInfo l_SubmitInfo{};
		l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		l_SubmitInfo.commandBufferCount = 1;
		l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;

		Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_Queue, 1, &l_SubmitInfo, l_Fence), "Failed vkQueueSubmit");
		Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &l_Fence, VK_TRUE, UINT64_MAX), "Failed vkWaitForFences");

		vkDestroyFence(m_Device, l_Fence, m_Allocator);
		vkDestroyCommandPool(m_Device, l_CommandPool, m_Allocator);
	}

	VulkanVertexBuffer::VulkanVertexBuffer(const VulkanContext& context, const VulkanDevice& device, uint64_t size, uint32_t stride,
		BufferMemoryUsage memoryUsage, const void* initialData) : m_Stride(stride)
	{
		if (stride == 0)
		{
			TR_CORE_CRITICAL("VulkanVertexBuffer stride=0");

			std::abort();
		}

		m_Buffer.Create(context, device, (VkDeviceSize)size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, memoryUsage);

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

	VulkanIndexBuffer::VulkanIndexBuffer(const VulkanContext& context, const VulkanDevice& device, uint64_t size, uint32_t indexCount,
		IndexType indexType, BufferMemoryUsage memoryUsage, const void* initialData) : m_IndexCount(indexCount), m_IndexType(indexType)
	{
		m_Buffer.Create(context, device, (VkDeviceSize)size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, memoryUsage);

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