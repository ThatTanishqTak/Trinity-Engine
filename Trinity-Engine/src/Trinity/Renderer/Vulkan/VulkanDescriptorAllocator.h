#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanDescriptorAllocator
	{
	public:
		VulkanDescriptorAllocator() = default;

		VulkanDescriptorAllocator(const VulkanDescriptorAllocator&) = delete;
		VulkanDescriptorAllocator& operator=(const VulkanDescriptorAllocator&) = delete;

		void Initialize(VkDevice device, VkAllocationCallbacks* allocator, uint32_t framesInFlight, uint32_t maxSetsPerFrame);
		void Shutdown();

		void BeginFrame(uint32_t frameIndex);
		VkDescriptorSet Allocate(uint32_t frameIndex, VkDescriptorSetLayout layout);

		bool IsInitialized() const { return m_Device != VK_NULL_HANDLE; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_HostAllocator = nullptr;
		std::vector<VkDescriptorPool> m_Pools;
	};
}