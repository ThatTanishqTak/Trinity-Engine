#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Trinity
{
	class VulkanDevice;

	class VulkanAllocator
	{
	public:
		VulkanAllocator() = default;
		~VulkanAllocator() = default;

		void Initialize(const VulkanDevice& device);
		void Shutdown();

		VmaAllocator GetAllocator() const { return m_Allocator; }

	private:
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
	};
}