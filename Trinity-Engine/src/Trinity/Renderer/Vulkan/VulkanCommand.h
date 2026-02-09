#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanContext;
	class VulkanDevice;

	class VulkanCommand
	{
	public:
		void Initialize(const VulkanContext& context, const VulkanDevice& device, uint32_t framesInFlight);
		void Shutdown();

		uint32_t GetFramesInFlight() const { return m_FramesInFlight; }

		VkCommandPool GetCommandPool(uint32_t frameIndex) const;
		VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const;

		void Reset(uint32_t frameIndex) const;

		void Begin(uint32_t frameIndex) const;
		void End(uint32_t frameIndex) const;

	private:
		void DestroyCommandObjects();
		void ValidateFrameIndex(uint32_t frameIndex) const;

	private:
		VkAllocationCallbacks* m_Allocator = nullptr;
		VkDevice m_Device = VK_NULL_HANDLE;

		uint32_t m_GraphicsQueueFamilyIndex = 0;
		uint32_t m_FramesInFlight = 0;

		std::vector<VkCommandPool> m_CommandPools{};
		std::vector<VkCommandBuffer> m_CommandBuffers{};
	};
}