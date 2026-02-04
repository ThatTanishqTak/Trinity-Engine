#include "Trinity/Renderer/Vulkan/VulkanSync.h"

#include "Trinity/Utilities/Utilities.h"

#include <cstdlib>

namespace Trinity
{
	void VulkanSync::Initialize(const VulkanContext& context, uint32_t maxFramesInFlight, uint32_t swapchainImageCount)
	{
		if (m_Device != VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("[VulkanSync] Initialize() called twice (Shutdown first)");

			std::abort();
		}

		if (maxFramesInFlight == 0 || swapchainImageCount == 0)
		{
			TR_CORE_CRITICAL("[VulkanSync] Initialize() invalid sizes: maxFramesInFlight={}, swapchainImageCount={}", maxFramesInFlight, swapchainImageCount);

			std::abort();
		}

		m_Device = context.Device;
		m_Allocator = context.Allocator;
		m_MaxFramesInFlight = maxFramesInFlight;

		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("[VulkanSync] Initialize() called with VK_NULL_HANDLE device.");

			std::abort();
		}

		CreateFrameSyncObjects();
		CreateSwapchainSyncObjects(swapchainImageCount);

		TR_CORE_TRACE("[VulkanSync] Initialized. FramesInFlight={}, SwapchainImages={}", m_MaxFramesInFlight, m_SwapchainImageCount);
	}

	void VulkanSync::Shutdown()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			m_ImageAvailableSemaphores.clear();
			m_InFlightFences.clear();
			m_RenderFinishedSemaphores.clear();
			m_ImagesInFlight.clear();
			m_MaxFramesInFlight = 0;
			m_SwapchainImageCount = 0;

			return;
		}

		DestroySwapchainSyncObjects();
		DestroyFrameSyncObjects();

		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;

		m_MaxFramesInFlight = 0;
		m_SwapchainImageCount = 0;
	}

	void VulkanSync::RecreateForSwapchain(uint32_t swapchainImageCount)
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("[VulkanSync] RecreateForSwapchain() called before Initialize()");

			std::abort();
		}

		if (swapchainImageCount == 0)
		{
			TR_CORE_CRITICAL("[VulkanSync] RecreateForSwapchain() called with swapchainImageCount == 0");

			std::abort();
		}

		DestroySwapchainSyncObjects();
		CreateSwapchainSyncObjects(swapchainImageCount);

		TR_CORE_TRACE("[VulkanSync] Recreated swapchain sync. SwapchainImages={}", m_SwapchainImageCount);
	}

	VkSemaphore VulkanSync::GetImageAvailableSemaphore(uint32_t frameIndex) const
	{
		if (frameIndex >= m_ImageAvailableSemaphores.size())
		{
			TR_CORE_CRITICAL("[VulkanSync] GetImageAvailableSemaphore out of range: {}", frameIndex);

			std::abort();
		}

		return m_ImageAvailableSemaphores[frameIndex];
	}

	VkSemaphore VulkanSync::GetRenderFinishedSemaphore(uint32_t imageIndex) const
	{
		if (imageIndex >= m_RenderFinishedSemaphores.size())
		{
			TR_CORE_CRITICAL("[VulkanSync] GetRenderFinishedSemaphore out of range: {}", imageIndex);

			std::abort();
		}

		return m_RenderFinishedSemaphores[imageIndex];
	}

	VkFence VulkanSync::GetInFlightFence(uint32_t frameIndex) const
	{
		if (frameIndex >= m_InFlightFences.size())
		{
			TR_CORE_CRITICAL("[VulkanSync] GetInFlightFence out of range: {}", frameIndex);

			std::abort();
		}

		return m_InFlightFences[frameIndex];
	}

	VkFence VulkanSync::GetImageInFlightFence(uint32_t imageIndex) const
	{
		if (imageIndex >= m_ImagesInFlight.size())
		{
			TR_CORE_CRITICAL("[VulkanSync] GetImageInFlightFence out of range: {}", imageIndex);

			std::abort();
		}

		return m_ImagesInFlight[imageIndex];
	}

	void VulkanSync::SetImageInFlightFence(uint32_t imageIndex, VkFence fence)
	{
		if (imageIndex >= m_ImagesInFlight.size())
		{
			TR_CORE_CRITICAL("[VulkanSync] SetImageInFlightFence out of range: {}", imageIndex);

			std::abort();
		}

		m_ImagesInFlight[imageIndex] = fence;
	}

	void VulkanSync::WaitForFrameFence(uint32_t frameIndex, uint64_t timeout) const
	{
		if (frameIndex >= m_InFlightFences.size())
		{
			TR_CORE_CRITICAL("[VulkanSync] WaitForFrameFence out of range: {}", frameIndex);

			std::abort();
		}

		const VkFence l_Fence = m_InFlightFences[frameIndex];
		if (l_Fence == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("[VulkanSync] WaitForFrameFence called with VK_NULL_HANDLE fence");

			std::abort();
		}

		Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &l_Fence, VK_TRUE, timeout), "Failed vkWaitForFences");
	}

	void VulkanSync::ResetFrameFence(uint32_t frameIndex) const
	{
		if (frameIndex >= m_InFlightFences.size())
		{
			TR_CORE_CRITICAL("[VulkanSync] ResetFrameFence out of range: {}", frameIndex);

			std::abort();
		}

		const VkFence l_Fence = m_InFlightFences[frameIndex];
		if (l_Fence == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("[VulkanSync] ResetFrameFence called with VK_NULL_HANDLE fence");

			std::abort();
		}

		Utilities::VulkanUtilities::VKCheck(vkResetFences(m_Device, 1, &l_Fence), "Failed vkResetFences");
	}

	void VulkanSync::CreateFrameSyncObjects()
	{
		m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight, VK_NULL_HANDLE);
		m_InFlightFences.resize(m_MaxFramesInFlight, VK_NULL_HANDLE);

		VkSemaphoreCreateInfo l_SemaphoreCreateInfo{};
		l_SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo l_FenceCreateInfo{};
		l_FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		l_FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t l_FrameIndex = 0; l_FrameIndex < m_MaxFramesInFlight; ++l_FrameIndex)
		{
			Utilities::VulkanUtilities::VKCheck(vkCreateSemaphore(m_Device, &l_SemaphoreCreateInfo, m_Allocator, &m_ImageAvailableSemaphores[l_FrameIndex]), "Failed vkCreateSemaphore (imageAvailable)");
			Utilities::VulkanUtilities::VKCheck(vkCreateFence(m_Device, &l_FenceCreateInfo, m_Allocator, &m_InFlightFences[l_FrameIndex]), "Failed vkCreateFence (inFlight)");
		}
	}

	void VulkanSync::DestroyFrameSyncObjects()
	{
		for (VkSemaphore& l_Semaphore : m_ImageAvailableSemaphores)
		{
			if (l_Semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device, l_Semaphore, m_Allocator);
				l_Semaphore = VK_NULL_HANDLE;
			}
		}

		for (VkFence& l_Fence : m_InFlightFences)
		{
			if (l_Fence != VK_NULL_HANDLE)
			{
				vkDestroyFence(m_Device, l_Fence, m_Allocator);
				l_Fence = VK_NULL_HANDLE;
			}
		}

		m_ImageAvailableSemaphores.clear();
		m_InFlightFences.clear();
	}

	void VulkanSync::CreateSwapchainSyncObjects(uint32_t swapchainImageCount)
	{
		m_SwapchainImageCount = swapchainImageCount;

		m_RenderFinishedSemaphores.resize(m_SwapchainImageCount, VK_NULL_HANDLE);
		m_ImagesInFlight.assign(m_SwapchainImageCount, VK_NULL_HANDLE);

		VkSemaphoreCreateInfo l_SemaphoreCreateInfo{};
		l_SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (uint32_t l_ImageIndex = 0; l_ImageIndex < m_SwapchainImageCount; ++l_ImageIndex)
		{
			Utilities::VulkanUtilities::VKCheck(vkCreateSemaphore(m_Device, &l_SemaphoreCreateInfo, m_Allocator, &m_RenderFinishedSemaphores[l_ImageIndex]), "Failed vkCreateSemaphore (renderFinished)");
		}
	}

	void VulkanSync::DestroySwapchainSyncObjects()
	{
		for (VkSemaphore& l_Semaphore : m_RenderFinishedSemaphores)
		{
			if (l_Semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device, l_Semaphore, m_Allocator);
				l_Semaphore = VK_NULL_HANDLE;
			}
		}

		m_RenderFinishedSemaphores.clear();
		m_ImagesInFlight.clear();
		m_SwapchainImageCount = 0;
	}
}