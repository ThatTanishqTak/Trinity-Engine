#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Trinity
{
	class VulkanContext;
	class VulkanDevice;

	class VulkanAllocator
	{
	public:
		void Initialize(const VulkanContext& context, const VulkanDevice& device);
		void Shutdown();

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocationFlags, VkBuffer& outBuffer,
			VmaAllocation& outAllocation);
		void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);


		void CreateImage(const VkImageCreateInfo& imageInfo, VkImage& outImage, VmaAllocation& outAllocation);
		void DestroyImage(VkImage image, VmaAllocation allocation);

		void* MapMemory(VmaAllocation allocation);
		void  UnmapMemory(VmaAllocation allocation);

		VmaAllocator GetHandle() const { return m_Allocator; }
		bool IsInitialized() const { return m_Allocator != VK_NULL_HANDLE; }

		VkDeviceSize GetMinUniformBufferOffsetAlignment() const;

	private:
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
	};
}