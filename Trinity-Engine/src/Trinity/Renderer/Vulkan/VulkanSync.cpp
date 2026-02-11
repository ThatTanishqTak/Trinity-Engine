#include "Trinity/Renderer/Vulkan/VulkanSync.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>

namespace Trinity
{
	void TransitionImage(VkCommandBuffer commandBuffer, VkImage image, const VulkanImageTransitionState& oldState, const VulkanImageTransitionState& newState,
		const VkImageSubresourceRange& subresourceRange)
	{
		VkImageMemoryBarrier2 l_MemoryBarrier{};
		l_MemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		l_MemoryBarrier.srcStageMask = oldState.m_StageMask;
		l_MemoryBarrier.srcAccessMask = oldState.m_AccessMask;
		l_MemoryBarrier.dstStageMask = newState.m_StageMask;
		l_MemoryBarrier.dstAccessMask = newState.m_AccessMask;
		l_MemoryBarrier.oldLayout = oldState.m_Layout;
		l_MemoryBarrier.newLayout = newState.m_Layout;
		l_MemoryBarrier.srcQueueFamilyIndex = oldState.m_QueueFamilyIndex;
		l_MemoryBarrier.dstQueueFamilyIndex = newState.m_QueueFamilyIndex;
		l_MemoryBarrier.image = image;
		l_MemoryBarrier.subresourceRange = subresourceRange;

		VkDependencyInfo l_DependencyInfo{};
		l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		l_DependencyInfo.imageMemoryBarrierCount = 1;
		l_DependencyInfo.pImageMemoryBarriers = &l_MemoryBarrier;

		vkCmdPipelineBarrier2(commandBuffer, &l_DependencyInfo);
	}

	void VulkanSync::Initialize(const VulkanContext& context, const VulkanDevice& device, uint32_t framesInFlight, uint32_t swapchainImageCount)
	{
		if (m_Device != VK_NULL_HANDLE)
		{
			TR_CORE_WARN("VulkanSync::Initialize called while already initialized. Reinitializing.");

			Shutdown();
		}

		const VkDevice l_Device = device.GetDevice();
		if (l_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanSync::Initialize called with invalid VkDevice");

			std::abort();
		}

		if (framesInFlight == 0)
		{
			TR_CORE_CRITICAL("VulkanSync::Initialize called with framesInFlight = 0");

			std::abort();
		}

		if (swapchainImageCount == 0)
		{
			TR_CORE_CRITICAL("VulkanSync::Initialize called with swapchainImageCount = 0");

			std::abort();
		}

		m_Device = l_Device;
		m_Allocator = context.GetAllocator();
		m_FramesInFlight = framesInFlight;
		m_SwapchainImageCount = swapchainImageCount;
		m_ImageAvailableSemaphores.resize(m_FramesInFlight, VK_NULL_HANDLE);
		m_InFlightFences.resize(m_FramesInFlight, VK_NULL_HANDLE);
		m_ImagesInFlight.resize(m_SwapchainImageCount, VK_NULL_HANDLE);
		m_RenderFinishedSemaphores.resize(m_SwapchainImageCount, VK_NULL_HANDLE);

		VkSemaphoreCreateInfo l_SemaphoreInfo{};
		l_SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo l_FenceInfo{};
		l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		l_FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t l_FrameIndex = 0; l_FrameIndex < m_FramesInFlight; ++l_FrameIndex)
		{
			Utilities::VulkanUtilities::VKCheck(vkCreateSemaphore(m_Device, &l_SemaphoreInfo, m_Allocator, &m_ImageAvailableSemaphores[l_FrameIndex]), "Failed vkCreateSemaphore");
			Utilities::VulkanUtilities::VKCheck(vkCreateFence(m_Device, &l_FenceInfo, m_Allocator, &m_InFlightFences[l_FrameIndex]), "Failed vkCreateFence");
		}

		CreateRenderFinishedSemaphores(m_SwapchainImageCount);

		TR_CORE_TRACE("VulkanSync initialized (FramesInFlight: {}, SwapchainImages: {})", m_FramesInFlight, m_SwapchainImageCount);
	}

	void VulkanSync::Shutdown()
	{
		DestroySyncObjects();

		m_ImagesInFlight.clear();
		m_ImagesInFlight.shrink_to_fit();
		m_FramesInFlight = 0;
		m_SwapchainImageCount = 0;
		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;
	}

	void VulkanSync::OnSwapchainRecreated(uint32_t swapchainImageCount)
	{
		if (swapchainImageCount == 0)
		{
			TR_CORE_CRITICAL("VulkanSync::OnSwapchainRecreated called with swapchainImageCount = 0");

			std::abort();
		}

		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanSync::OnSwapchainRecreated called before Initialize");

			std::abort();
		}

		m_SwapchainImageCount = swapchainImageCount;

		DestroyRenderFinishedSemaphores();
		m_RenderFinishedSemaphores.resize(m_SwapchainImageCount, VK_NULL_HANDLE);
		CreateRenderFinishedSemaphores(m_SwapchainImageCount);

		m_ImagesInFlight.assign(m_SwapchainImageCount, VK_NULL_HANDLE);

		TR_CORE_TRACE("VulkanSync updated for swapchain recreation (SwapchainImages: {})", m_SwapchainImageCount);
	}

	VkSemaphore VulkanSync::GetImageAvailableSemaphore(uint32_t frameIndex) const
	{
		ValidateFrameIndex(frameIndex);
		return m_ImageAvailableSemaphores[frameIndex];
	}

	VkSemaphore VulkanSync::GetRenderFinishedSemaphore(uint32_t imageIndex) const
	{
		ValidateImageIndex(imageIndex);
		return m_RenderFinishedSemaphores[imageIndex];
	}

	VkFence VulkanSync::GetInFlightFence(uint32_t frameIndex) const
	{
		ValidateFrameIndex(frameIndex);
		return m_InFlightFences[frameIndex];
	}

	void VulkanSync::WaitForFrameFence(uint32_t frameIndex, uint64_t timeout) const
	{
		ValidateFrameIndex(frameIndex);

		const VkFence l_Fence = m_InFlightFences[frameIndex];
		if (l_Fence == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanSync::WaitForFrameFence called with null fence");

			std::abort();
		}

		Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &l_Fence, VK_TRUE, timeout), "Failed vkWaitForFences");
	}

	void VulkanSync::ResetFrameFence(uint32_t frameIndex) const
	{
		ValidateFrameIndex(frameIndex);

		const VkFence l_Fence = m_InFlightFences[frameIndex];
		if (l_Fence == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanSync::ResetFrameFence called with null fence");

			std::abort();
		}

		Utilities::VulkanUtilities::VKCheck(vkResetFences(m_Device, 1, &l_Fence), "Failed vkResetFences");
	}

	VkFence VulkanSync::GetImageInFlightFence(uint32_t imageIndex) const
	{
		ValidateImageIndex(imageIndex);

		return m_ImagesInFlight[imageIndex];
	}

	void VulkanSync::SetImageInFlightFence(uint32_t imageIndex, VkFence fence)
	{
		ValidateImageIndex(imageIndex);
		m_ImagesInFlight[imageIndex] = fence;
	}

	void VulkanSync::ClearImagesInFlight()
	{
		for (uint32_t l_ImageIndex = 0; l_ImageIndex < static_cast<uint32_t>(m_ImagesInFlight.size()); ++l_ImageIndex)
		{
			m_ImagesInFlight[l_ImageIndex] = VK_NULL_HANDLE;
		}
	}

	void VulkanSync::DestroyRenderFinishedSemaphores()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		for (VkSemaphore it_Semaphore : m_RenderFinishedSemaphores)
		{
			if (it_Semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device, it_Semaphore, m_Allocator);
			}
		}

		m_RenderFinishedSemaphores.clear();
	}

	void VulkanSync::CreateRenderFinishedSemaphores(uint32_t swapchainImageCount)
	{
		if (swapchainImageCount == 0)
		{
			TR_CORE_CRITICAL("VulkanSync::CreateRenderFinishedSemaphores called with swapchainImageCount = 0");
			std::abort();
		}

		VkSemaphoreCreateInfo l_SemaphoreInfo{};
		l_SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (uint32_t l_ImageIndex = 0; l_ImageIndex < swapchainImageCount; ++l_ImageIndex)
		{
			Utilities::VulkanUtilities::VKCheck(vkCreateSemaphore(m_Device, &l_SemaphoreInfo, m_Allocator, &m_RenderFinishedSemaphores[l_ImageIndex]), "Failed vkCreateSemaphore");
		}
	}

	void VulkanSync::DestroySyncObjects()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		for (VkFence it_Fence : m_InFlightFences)
		{
			if (it_Fence != VK_NULL_HANDLE)
			{
				vkDestroyFence(m_Device, it_Fence, m_Allocator);
			}
		}

		DestroyRenderFinishedSemaphores();

		for (VkSemaphore it_Semaphore : m_ImageAvailableSemaphores)
		{
			if (it_Semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device, it_Semaphore, m_Allocator);
			}
		}

		m_ImageAvailableSemaphores.clear();
		m_InFlightFences.clear();

		m_ImageAvailableSemaphores.shrink_to_fit();
		m_InFlightFences.shrink_to_fit();
		m_RenderFinishedSemaphores.shrink_to_fit();
	}

	void VulkanSync::ValidateFrameIndex(uint32_t frameIndex) const
	{
		if (frameIndex >= m_FramesInFlight)
		{
			TR_CORE_CRITICAL("VulkanSync frame index out of range ({} >= {})", frameIndex, m_FramesInFlight);

			std::abort();
		}
	}

	void VulkanSync::ValidateImageIndex(uint32_t imageIndex) const
	{
		if (imageIndex >= m_SwapchainImageCount)
		{
			TR_CORE_CRITICAL("VulkanSync swapchain image index out of range ({} >= {})", imageIndex, m_SwapchainImageCount);

			std::abort();
		}
	}
}