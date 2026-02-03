#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanDevice;

	class VulkanFrameResources
	{
	public:
		struct PerFrameResources
		{
			VkFence InFlightFence = VK_NULL_HANDLE;
			VkSemaphore ImageAvailable = VK_NULL_HANDLE;
			VkSemaphore RenderFinished = VK_NULL_HANDLE;
		};

	public:
		VulkanFrameResources() = default;
		~VulkanFrameResources() = default;

		VulkanFrameResources(const VulkanFrameResources&) = delete;
		VulkanFrameResources& operator=(const VulkanFrameResources&) = delete;
		VulkanFrameResources(VulkanFrameResources&&) = delete;
		VulkanFrameResources& operator=(VulkanFrameResources&&) = delete;

		void Initialize(const VulkanDevice& device, uint32_t framesInFlight, VkAllocationCallbacks* allocator = nullptr);
		void Shutdown();

		uint32_t GetFrameCount() const { return static_cast<uint32_t>(m_Frames.size()); }
		uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

		PerFrameResources& GetCurrentFrame();
		const PerFrameResources& GetCurrentFrame() const;

		PerFrameResources& GetFrame(uint32_t frameIndex);
		const PerFrameResources& GetFrame(uint32_t frameIndex) const;

		void AdvanceFrame();

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;

		std::vector<PerFrameResources> m_Frames;
		uint32_t m_CurrentFrameIndex = 0;
	};
}