#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
    class VulkanDevice;

	class VulkanCommandPool
	{
	public:
        VulkanCommandPool() = default;
        ~VulkanCommandPool() = default;

        void Initialize(VulkanDevice& device, uint32_t framesInFlight);
        void Shutdown();

        VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const { return m_CommandBuffers[frameIndex]; }
        VkCommandPool GetPool() const { return m_CommandPool; }

        void ResetCommandBuffer(uint32_t frameIndex) const;
        void BeginCommandBuffer(uint32_t frameIndex) const;
        void EndCommandBuffer(uint32_t frameIndex) const;

        VkCommandBuffer BeginSingleTimeCommands() const;
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;

	private:
		VulkanDevice* m_Device = nullptr;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_CommandBuffers;
	};
}