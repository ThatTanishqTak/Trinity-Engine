#pragma once

#include <vulkan/vulkan.h>

namespace Trinity
{
	class VulkanCommand;
	class VulkanDevice;

	class VulkanBuffer
	{
	public:
		VulkanBuffer() = default;
		~VulkanBuffer() = default;

		VulkanBuffer(const VulkanBuffer&) = delete;
		VulkanBuffer& operator=(const VulkanBuffer&) = delete;
		VulkanBuffer(VulkanBuffer&&) = delete;
		VulkanBuffer& operator=(VulkanBuffer&&) = delete;

		void Create(const VulkanDevice& deviceRef, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProps);
		void Destroy();

		void* Map();
		void Unmap();

		void CopyFromStaging(const VulkanCommand& command, const VulkanBuffer& stagingBuffer, VkQueue queue);

		VkBuffer GetBuffer() const { return m_Buffer; }
		VkDeviceSize GetSize() const { return m_Size; }
		VkBufferUsageFlags GetUsage() const { return m_Usage; }
		VkMemoryPropertyFlags GetMemoryProperties() const { return m_MemoryProperties; }

	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;
		VkDeviceSize m_Size = 0;
		VkBufferUsageFlags m_Usage = 0;
		VkMemoryPropertyFlags m_MemoryProperties = 0;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;
	};
}