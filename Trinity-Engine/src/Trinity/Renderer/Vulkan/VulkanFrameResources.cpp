#include "Trinity/Renderer/Vulkan/VulkanFrameResources.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Utilities/Utilities.h"

#include <cstdlib>

namespace Trinity
{
	void VulkanFrameResources::Initialize(const VulkanDevice& device, uint32_t framesInFlight, VkAllocationCallbacks* allocator)
	{
		if (framesInFlight == 0)
		{
			TR_CORE_CRITICAL("VulkanFrameResources::Initialize called with framesInFlight == 0");

			std::abort();
		}

		m_Device = device.GetDevice();
		m_Allocator = allocator;

		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanFrameResources::Initialize called with invalid VkDevice");

			std::abort();
		}

		m_Frames.clear();
		m_Frames.resize(framesInFlight);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			PerFrameResources& l_Frame = m_Frames[i];

			// Fence (signaled so first frame doesn't stall)
			VkFenceCreateInfo l_FenceInfo{};
			l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			l_FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			Utilities::VulkanUtilities::VKCheck(vkCreateFence(m_Device, &l_FenceInfo, m_Allocator, &l_Frame.InFlightFence), "Failed vkCreateFence");

			// Semaphores
			VkSemaphoreCreateInfo l_SemInfo{};
			l_SemInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			Utilities::VulkanUtilities::VKCheck(vkCreateSemaphore(m_Device, &l_SemInfo, m_Allocator, &l_Frame.ImageAvailable), "Failed vkCreateSemaphore (ImageAvailable)");
			Utilities::VulkanUtilities::VKCheck(vkCreateSemaphore(m_Device, &l_SemInfo, m_Allocator, &l_Frame.RenderFinished), "Failed vkCreateSemaphore (RenderFinished)");
		}

		m_CurrentFrameIndex = 0;
	}

	void VulkanFrameResources::Shutdown()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			m_Frames.clear();

			return;
		}

		vkDeviceWaitIdle(m_Device);

		for (PerFrameResources& l_Frame : m_Frames)
		{
			if (l_Frame.RenderFinished != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device, l_Frame.RenderFinished, m_Allocator);
			}

			if (l_Frame.ImageAvailable != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device, l_Frame.ImageAvailable, m_Allocator);
			}

			if (l_Frame.InFlightFence != VK_NULL_HANDLE)
			{
				vkDestroyFence(m_Device, l_Frame.InFlightFence, m_Allocator);
			}

			l_Frame = {};
		}

		m_Frames.clear();

		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;
		m_CurrentFrameIndex = 0;
	}

	VulkanFrameResources::PerFrameResources& VulkanFrameResources::GetCurrentFrame()
	{
		if (m_Frames.empty())
		{
			TR_CORE_CRITICAL("VulkanFrameResources::GetCurrentFrame called with no frames");

			std::abort();
		}

		return m_Frames[m_CurrentFrameIndex];
	}

	const VulkanFrameResources::PerFrameResources& VulkanFrameResources::GetCurrentFrame() const
	{
		if (m_Frames.empty())
		{
			TR_CORE_CRITICAL("VulkanFrameResources::GetCurrentFrame const called with no frames");

			std::abort();
		}

		return m_Frames[m_CurrentFrameIndex];
	}

	VulkanFrameResources::PerFrameResources& VulkanFrameResources::GetFrame(uint32_t frameIndex)
	{
		if (frameIndex >= m_Frames.size())
		{
			TR_CORE_CRITICAL("VulkanFrameResources::GetFrame invalid frameIndex {}", frameIndex);

			std::abort();
		}

		return m_Frames[frameIndex];
	}

	const VulkanFrameResources::PerFrameResources& VulkanFrameResources::GetFrame(uint32_t frameIndex) const
	{
		if (frameIndex >= m_Frames.size())
		{
			TR_CORE_CRITICAL("VulkanFrameResources::GetFrame const invalid frameIndex {}", frameIndex);

			std::abort();
		}

		return m_Frames[frameIndex];
	}

	void VulkanFrameResources::AdvanceFrame()
	{
		if (m_Frames.empty())
		{
			TR_CORE_CRITICAL("VulkanFrameResources::AdvanceFrame called with no frames");

			std::abort();
		}

		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % static_cast<uint32_t>(m_Frames.size());
	}
}