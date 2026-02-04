#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"

#include "Trinity/Renderer/Vulkan/VulkanCommand.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Utilities/Utilities.h"

#include <algorithm>
#include <cstdlib>

namespace Trinity
{
	void VulkanBuffer::Create(const VulkanDevice& deviceRef, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProps)
	{
		if (m_Buffer != VK_NULL_HANDLE || m_Memory != VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Create called on existing buffer");

			std::abort();
		}

		m_Device = deviceRef.GetDevice();
		m_Allocator = nullptr;
		m_Size = size;
		m_Usage = usage;
		m_MemoryProperties = memProps;

		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Create called with invalid device");

			std::abort();
		}

		VkBufferCreateInfo l_BufferInfo{};
		l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		l_BufferInfo.size = size;
		l_BufferInfo.usage = usage;
		l_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		Utilities::VulkanUtilities::VKCheck(vkCreateBuffer(m_Device, &l_BufferInfo, m_Allocator, &m_Buffer), "Failed vkCreateBuffer");

		VkMemoryRequirements l_MemRequirements{};
		vkGetBufferMemoryRequirements(m_Device, m_Buffer, &l_MemRequirements);

		VkMemoryAllocateInfo l_AllocInfo{};
		l_AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		l_AllocInfo.allocationSize = l_MemRequirements.size;
		l_AllocInfo.memoryTypeIndex = deviceRef.FindMemoryType(l_MemRequirements.memoryTypeBits, memProps);

		Utilities::VulkanUtilities::VKCheck(vkAllocateMemory(m_Device, &l_AllocInfo, m_Allocator, &m_Memory), "Failed vkAllocateMemory");
		Utilities::VulkanUtilities::VKCheck(vkBindBufferMemory(m_Device, m_Buffer, m_Memory, 0), "Failed vkBindBufferMemory");
	}

	void VulkanBuffer::Destroy()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			m_Buffer = VK_NULL_HANDLE;
			m_Memory = VK_NULL_HANDLE;
			m_Size = 0;
			m_Usage = 0;
			m_MemoryProperties = 0;

			return;
		}

		if (m_Buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(m_Device, m_Buffer, m_Allocator);
		}

		if (m_Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_Device, m_Memory, m_Allocator);
		}

		m_Buffer = VK_NULL_HANDLE;
		m_Memory = VK_NULL_HANDLE;
		m_Size = 0;
		m_Usage = 0;
		m_MemoryProperties = 0;
		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;
	}

	void* VulkanBuffer::Map()
	{
		if ((m_MemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Map called on non-host-visible buffer");

			std::abort();
		}

		if (m_Device == VK_NULL_HANDLE || m_Memory == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Map called with invalid device or memory");

			std::abort();
		}

		void* l_Data = nullptr;
		Utilities::VulkanUtilities::VKCheck(vkMapMemory(m_Device, m_Memory, 0, m_Size, 0, &l_Data), "Failed vkMapMemory");

		return l_Data;
	}

	void VulkanBuffer::Unmap()
	{
		if ((m_MemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Unmap called on non-host-visible buffer");

			std::abort();
		}

		if (m_Device == VK_NULL_HANDLE || m_Memory == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanBuffer::Unmap called with invalid device or memory");

			std::abort();
		}

		vkUnmapMemory(m_Device, m_Memory);
	}

	void VulkanBuffer::CopyFromStaging(const VulkanCommand& command, const VulkanBuffer& stagingBuffer, VkQueue queue)
	{
		if (m_Device == VK_NULL_HANDLE || m_Buffer == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanBuffer::CopyFromStaging called with invalid destination buffer");

			std::abort();
		}

		if (stagingBuffer.m_Buffer == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanBuffer::CopyFromStaging called with invalid staging buffer");

			std::abort();
		}

		VkCommandBuffer l_CommandBuffer = command.BeginSingleTime(command.GetUploadCommandPool());

		VkBufferCopy l_Region{};
		l_Region.srcOffset = 0;
		l_Region.dstOffset = 0;
		l_Region.size = std::min(m_Size, stagingBuffer.m_Size);

		vkCmdCopyBuffer(l_CommandBuffer, stagingBuffer.m_Buffer, m_Buffer, 1, &l_Region);

		command.EndSingleTime(l_CommandBuffer, command.GetUploadCommandPool(), queue);
	}
}