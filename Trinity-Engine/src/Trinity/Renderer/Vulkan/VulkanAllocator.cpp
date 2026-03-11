#define VMA_IMPLEMENTATION
#include "Trinity/Renderer/Vulkan/VulkanAllocator.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>

namespace Trinity
{
	void VulkanAllocator::Initialize(const VulkanContext& context, const VulkanDevice& device)
	{
		if (m_Allocator != VK_NULL_HANDLE)
		{
			TR_CORE_WARN("VulkanAllocator::Initialize called while already initialized");

			return;
		}

		VmaAllocatorCreateInfo l_AllocatorInfo{};
		l_AllocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		l_AllocatorInfo.instance = context.GetInstance();
		l_AllocatorInfo.physicalDevice = device.GetPhysicalDevice();
		l_AllocatorInfo.device = device.GetDevice();
		l_AllocatorInfo.pAllocationCallbacks = context.GetAllocator();

		Utilities::VulkanUtilities::VKCheck(vmaCreateAllocator(&l_AllocatorInfo, &m_Allocator), "Failed vmaCreateAllocator");

		TR_CORE_TRACE("VulkanAllocator Initialized");
	}

	void VulkanAllocator::Shutdown()
	{
		if (m_Allocator == VK_NULL_HANDLE)
		{
			return;
		}

		vmaDestroyAllocator(m_Allocator);
		m_Allocator = VK_NULL_HANDLE;

		TR_CORE_TRACE("VulkanAllocator Shutdown");
	}

	void VulkanAllocator::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocationFlags, VkBuffer& outBuffer,
		VmaAllocation& outAllocation)
	{
		VkBufferCreateInfo l_BufferInfo{};
		l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		l_BufferInfo.size = size;
		l_BufferInfo.usage = bufferUsage;
		l_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo l_AllocInfo{};
		l_AllocInfo.usage = memoryUsage;
		l_AllocInfo.flags = allocationFlags;

		Utilities::VulkanUtilities::VKCheck(vmaCreateBuffer(m_Allocator, &l_BufferInfo, &l_AllocInfo, &outBuffer, &outAllocation, nullptr), "Failed vmaCreateBuffer");
	}

	void VulkanAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
		if (buffer == VK_NULL_HANDLE || allocation == VK_NULL_HANDLE)
		{
			return;
		}

		vmaDestroyBuffer(m_Allocator, buffer, allocation);
	}

	void VulkanAllocator::CreateImage(const VkImageCreateInfo& imageInfo, VkImage& outImage, VmaAllocation& outAllocation)
	{
		VmaAllocationCreateInfo l_AllocInfo{};
		l_AllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		Utilities::VulkanUtilities::VKCheck(vmaCreateImage(m_Allocator, &imageInfo, &l_AllocInfo, &outImage, &outAllocation, nullptr), "Failed vmaCreateImage");
	}

	void VulkanAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
	{
		if (image == VK_NULL_HANDLE || allocation == VK_NULL_HANDLE)
		{
			return;
		}

		vmaDestroyImage(m_Allocator, image, allocation);
	}

	void* VulkanAllocator::MapMemory(VmaAllocation allocation)
	{
		void* l_Pointer = nullptr;
		Utilities::VulkanUtilities::VKCheck(vmaMapMemory(m_Allocator, allocation, &l_Pointer), "Failed vmaMapMemory");

		return l_Pointer;
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation)
	{
		vmaUnmapMemory(m_Allocator, allocation);
	}

	VkDeviceSize VulkanAllocator::GetMinUniformBufferOffsetAlignment() const
	{
		const VkPhysicalDeviceProperties* l_Props = nullptr;
		vmaGetPhysicalDeviceProperties(m_Allocator, &l_Props);

		return l_Props->limits.minUniformBufferOffsetAlignment;
	}
}