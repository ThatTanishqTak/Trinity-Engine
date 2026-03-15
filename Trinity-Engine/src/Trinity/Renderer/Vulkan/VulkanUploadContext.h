#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

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

		void UploadImage(VkBuffer stagingBuffer, VkImage dstImage, uint32_t width, uint32_t height);
		void UploadImageWithMips(VkBuffer stagingBuffer, VkImage dstImage, uint32_t width, uint32_t height, uint32_t mipLevels);

		bool IsInitialized() const { return m_Device != VK_NULL_HANDLE; }

	private:
		void BeginCommands();
		void SubmitAndWait();

	private:
		VkAllocationCallbacks* m_Allocator = nullptr;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_Queue = VK_NULL_HANDLE;
		VkCommandPool m_Pool = VK_NULL_HANDLE;
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
		VkFence m_Fence = VK_NULL_HANDLE;
	};
}