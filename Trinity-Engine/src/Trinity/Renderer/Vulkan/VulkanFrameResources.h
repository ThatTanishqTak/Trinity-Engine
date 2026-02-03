#pragma once

#include <cstdint>

namespace Trinity
{
	class VulkanFrameResources
	{
	public:
		VulkanFrameResources() = default;
		~VulkanFrameResources() = default;

		VulkanFrameResources(const VulkanFrameResources&) = delete;
		VulkanFrameResources& operator=(const VulkanFrameResources&) = delete;
		VulkanFrameResources(VulkanFrameResources&&) = delete;
		VulkanFrameResources& operator=(VulkanFrameResources&&) = delete;

		void Initialize(uint32_t framesInFlight);
		void Shutdown();

		uint32_t GetFrameCount() const { return m_FrameCount; }
		uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

		void AdvanceFrame();
		void Reset();

	private:
		uint32_t m_FrameCount = 0;
		uint32_t m_CurrentFrameIndex = 0;
	};
}