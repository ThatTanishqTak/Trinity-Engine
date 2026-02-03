#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanDevice;

	class VulkanCommand
	{
	public:
		struct PerFrameCommand
		{
			VkCommandPool CommandPool = VK_NULL_HANDLE;
			VkCommandBuffer PrimaryCommandBuffer = VK_NULL_HANDLE;
		};

	public:
		VulkanCommand() = default;
		~VulkanCommand() = default;

		VulkanCommand(const VulkanCommand&) = delete;
		VulkanCommand& operator=(const VulkanCommand&) = delete;
		VulkanCommand(VulkanCommand&&) = delete;
		VulkanCommand& operator=(VulkanCommand&&) = delete;

		void Initialize(const VulkanDevice& device, uint32_t framesInFlight, VkAllocationCallbacks* allocator = nullptr);
		void Shutdown();

		// Per-frame recording helpers
		VkCommandBuffer BeginFrame(uint32_t frameIndex, VkCommandBufferUsageFlags usageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		void EndFrame(uint32_t frameIndex);

		VkCommandPool GetCommandPool(uint32_t frameIndex) const;
		VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const;

		// One-off command buffer helpers (great for uploads during init)
		VkCommandPool GetUploadCommandPool() const { return m_UploadCommandPool; }

		VkCommandBuffer BeginSingleTime(VkCommandPool commandPool) const;
		void EndSingleTime(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue) const;

	private:
		void CreatePerFrame(uint32_t framesInFlight);
		void DestroyPerFrame();

		void CreateUploadPool();
		void DestroyUploadPool();

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		uint32_t m_GraphicsQueueFamilyIndex = UINT32_MAX;
		uint32_t m_TransferQueueFamilyIndex = UINT32_MAX;
		VkAllocationCallbacks* m_Allocator = nullptr;

		std::vector<PerFrameCommand> m_Frames;

		// Transient pool for one-time command buffers (uploads, staging copies, etc.)
		VkCommandPool m_UploadCommandPool = VK_NULL_HANDLE;
	};
}