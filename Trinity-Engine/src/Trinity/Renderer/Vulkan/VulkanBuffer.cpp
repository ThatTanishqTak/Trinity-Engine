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

		m_TransferPool = other.m_TransferPool;
		m_TransferCmdBuffer = other.m_TransferCmdBuffer;
		m_TransferFence = other.m_TransferFence;

		other.m_Allocator = nullptr;
		other.m_PhysicalDevice = VK_NULL_HANDLE;
		other.m_Device = VK_NULL_HANDLE;
		other.m_Queue = VK_NULL_HANDLE;
		other.m_QueueFamilyIndex = 0;
		other.m_Buffer = VK_NULL_HANDLE;
		other.m_Memory = VK_NULL_HANDLE;
		other.m_Size = 0;
		other.m_Mapped = nullptr;

		other.m_TransferPool = VK_NULL_HANDLE;
		other.m_TransferCmdBuffer = VK_NULL_HANDLE;
		other.m_TransferFence = VK_NULL_HANDLE;

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

		if (memoryUsage != BufferMemoryUsage::CPUToGPU)
		{
			VkCommandPoolCreateInfo l_PoolInfo{};
			l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			l_PoolInfo.queueFamilyIndex = m_QueueFamilyIndex;
			Utilities::VulkanUtilities::VKCheck(vkCreateCommandPool(m_Device, &l_PoolInfo, m_Allocator, &m_TransferPool), "Failed vkCreateCommandPool");

			VkCommandBufferAllocateInfo l_AllocInfo{};
			l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			l_AllocInfo.commandPool = m_TransferPool;
			l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			l_AllocInfo.commandBufferCount = 1;
			Utilities::VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_Device, &l_AllocInfo, &m_TransferCmdBuffer), "Failed vkAllocateCommandBuffers");

			VkFenceCreateInfo l_FenceInfo{};
			l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			l_FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			Utilities::VulkanUtilities::VKCheck(vkCreateFence(m_Device, &l_FenceInfo, m_Allocator, &m_TransferFence), "Failed vkCreateFence");
		}
	}

	void VulkanBuffer::Destroy()
	{
		if (m_Device == VK_NULL_HANDLE)
			return;

		Unmap();

		if (m_TransferFence != VK_NULL_HANDLE)
		{
			vkDestroyFence(m_Device, m_TransferFence, m_Allocator);
			m_TransferFence = VK_NULL_HANDLE;
		}

		// Destroying the pool implicitly frees m_TransferCmdBuffer.
		if (m_TransferPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(m_Device, m_TransferPool, m_Allocator);
			m_TransferPool = VK_NULL_HANDLE;
			m_TransferCmdBuffer = VK_NULL_HANDLE;
		}

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

	VulkanBuffer VulkanBuffer::CreateStaging(const VulkanBuffer& source, VkDeviceSize size)
	{
		VulkanBuffer l_Staging{};
		l_Staging.m_Allocator = source.m_Allocator;
		l_Staging.m_PhysicalDevice = source.m_PhysicalDevice;
		l_Staging.m_Device = source.m_Device;
		l_Staging.m_Queue = source.m_Queue;
		l_Staging.m_QueueFamilyIndex = source.m_QueueFamilyIndex;
		l_Staging.m_Size = size;
		l_Staging.m_MemoryUsage = BufferMemoryUsage::CPUToGPU;
		l_Staging.CreateRawBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		return l_Staging;
	}

	void VulkanBuffer::SetData(const void* data, VkDeviceSize size, VkDeviceSize offset)
	{
		if (!data)
		{
			TR_CORE_CRITICAL("VulkanBuffer::SetData null data");

			std::abort();
		}

		if (size == 0)
		{
			// Nothing to do.
			return;
		}

		if (offset + size > m_Size)
		{
			TR_CORE_CRITICAL("VulkanBuffer::SetData OOB");

			std::abort();
		}

		if (m_MemoryUsage == BufferMemoryUsage::CPUToGPU)
		{
			void* l_DestinationBuffer = Map(offset);
			std::memcpy(l_DestinationBuffer, data, (size_t)size);
			Unmap();

			return;
		}

		VulkanBuffer l_StagingBuffer = CreateStaging(*this, size);

		void* l_Map = l_StagingBuffer.Map(0);
		std::memcpy(l_Map, data, (size_t)size);
		l_StagingBuffer.Unmap();

		CopyBufferImmediate(l_StagingBuffer.m_Buffer, m_Buffer, size, offset, 0);
	}

	void* VulkanBuffer::Map(VkDeviceSize offset)
	{
		if (m_MemoryUsage != BufferMemoryUsage::CPUToGPU)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Map on GPUOnly buffer");

			std::abort();
		}

		if (!m_Mapped)
		{
			Utilities::VulkanUtilities::VKCheck(vkMapMemory(m_Device, m_Memory, 0, VK_WHOLE_SIZE, 0, &m_Mapped), "Failed vkMapMemory");
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

	void VulkanBuffer::CopyBufferImmediate(VkBuffer sourceBuffer, VkBuffer l_DestinationBuffer, VkDeviceSize size, VkDeviceSize dstOffset, VkDeviceSize srcOffset)
	{
		Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &m_TransferFence, VK_TRUE, UINT64_MAX), "Failed vkWaitForFences");
		Utilities::VulkanUtilities::VKCheck(vkResetFences(m_Device, 1, &m_TransferFence), "Failed vkResetFences");
		Utilities::VulkanUtilities::VKCheck(vkResetCommandPool(m_Device, m_TransferPool, 0), "Failed vkResetCommandPool");

		VkCommandBufferBeginInfo l_CommandBufferBeginInfo{};
		l_CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		l_CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		Utilities::VulkanUtilities::VKCheck(vkBeginCommandBuffer(m_TransferCmdBuffer, &l_CommandBufferBeginInfo), "Failed vkBeginCommandBuffer");

		VkBufferCopy l_BufferCopy{};
		l_BufferCopy.srcOffset = srcOffset;
		l_BufferCopy.dstOffset = dstOffset;
		l_BufferCopy.size = size;
		vkCmdCopyBuffer(m_TransferCmdBuffer, sourceBuffer, l_DestinationBuffer, 1, &l_BufferCopy);

		Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(m_TransferCmdBuffer), "Failed vkEndCommandBuffer");

		VkSubmitInfo l_SubmitInfo{};
		l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		l_SubmitInfo.commandBufferCount = 1;
		l_SubmitInfo.pCommandBuffers = &m_TransferCmdBuffer;

		Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_Queue, 1, &l_SubmitInfo, m_TransferFence), "Failed vkQueueSubmit");
		Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &m_TransferFence, VK_TRUE, UINT64_MAX), "Failed vkWaitForFences");
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