#include "Trinity/Renderer/Vulkan/VulkanFrameResources.h"

#include "Trinity/Utilities/Utilities.h"

#include <cstdlib>

namespace Trinity
{
	void VulkanFrameResources::Initialize(uint32_t framesInFlight)
	{
		if (framesInFlight == 0)
		{
			TR_CORE_CRITICAL("VulkanFrameResources::Initialize called with framesInFlight == 0");

			std::abort();
		}

		m_FrameCount = framesInFlight;
		m_CurrentFrameIndex = 0;
	}

	void VulkanFrameResources::Shutdown()
	{
		m_FrameCount = 0;
		m_CurrentFrameIndex = 0;
	}

	void VulkanFrameResources::AdvanceFrame()
	{
		if (m_FrameCount == 0)
		{
			TR_CORE_CRITICAL("VulkanFrameResources::AdvanceFrame called with no frames");

			std::abort();
		}

		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_FrameCount;
	}

	void VulkanFrameResources::Reset()
	{
		m_CurrentFrameIndex = 0;
	}
}