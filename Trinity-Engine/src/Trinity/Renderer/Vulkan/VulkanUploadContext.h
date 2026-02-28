#pragma once

#include <vulkan/vulkan.h>

namespace Trinity
{
	class VulkanContext;
	class VulkanDevice;

	class VulkanUploadContext
	{
	public:
		VulkanUploadContext() = default;

		VulkanUploadContext(const VulkanUploadContext&) = delete;
		VulkanUploadContext& operator=(const VulkanUploadContext&) = delete;

		void Initialize(const VulkanContext& context, const VulkanDevice& device);
		void Shutdown();

		void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

		bool IsInitialized() const { return m_Device != VK_NULL_HANDLE; }

	private:
		VkAllocationCallbacks* m_Allocator = nullptr;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_Queue = VK_NULL_HANDLE;
		VkCommandPool m_Pool = VK_NULL_HANDLE;
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
		VkFence m_Fence = VK_NULL_HANDLE;
	};
}