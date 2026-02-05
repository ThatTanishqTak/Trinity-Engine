#include "Trinity/Renderer/Vulkan/VulkanCommand.h"

#include "Trinity/Utilities/Utilities.h"

#include <cstdlib>
#include <limits>

namespace Trinity
{
	void VulkanCommand::Initialize(const VulkanContext& context, uint32_t framesInFlight)
	{
		if (m_Device != VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanCommand::Initialize called twice (Shutdown first)");

			std::abort();
		}

		if (framesInFlight == 0)
		{
			TR_CORE_CRITICAL("VulkanCommand::Initialize called with framesInFlight == 0");

			std::abort();
		}

		m_Device = context.Device;
		m_Allocator = context.Allocator;
		m_GraphicsQueueFamilyIndex = context.Queues.GraphicsFamilyIndex;
		m_TransferQueueFamilyIndex = m_GraphicsQueueFamilyIndex;
		m_UploadQueue = context.Queues.GraphicsQueue;

		if (m_Device == VK_NULL_HANDLE || m_GraphicsQueueFamilyIndex == UINT32_MAX)
		{
			TR_CORE_CRITICAL("VulkanCommand::Initialize called with invalid device or graphics queue family index");

			std::abort();
		}

		// If no dedicated transfer queue exists, fall back to graphics.
		if (m_TransferQueueFamilyIndex == UINT32_MAX)
		{
			m_TransferQueueFamilyIndex = m_GraphicsQueueFamilyIndex;
		}

		if (m_UploadQueue == VK_NULL_HANDLE)
		{
			m_UploadQueue = context.Queues.GraphicsQueue;
		}

		if (m_UploadQueue == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanCommand::Initialize called with invalid upload queue");

			std::abort();
		}

		CreatePerFrame(framesInFlight);
		CreateUploadPool();
	}

	void VulkanCommand::Shutdown()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			m_Frames.clear();

			return;
		}

		DestroyUploadPool();
		DestroyPerFrame();

		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;
		m_GraphicsQueueFamilyIndex = UINT32_MAX;
		m_TransferQueueFamilyIndex = UINT32_MAX;
		m_UploadQueue = VK_NULL_HANDLE;
	}

	VkCommandBuffer VulkanCommand::BeginFrame(uint32_t frameIndex, VkCommandBufferUsageFlags usageFlags)
	{
		if (frameIndex >= m_Frames.size())
		{
			TR_CORE_CRITICAL("VulkanCommand::BeginFrame invalid frameIndex {}", frameIndex);

			std::abort();
		}

		PerFrameCommand& l_Frame = m_Frames[frameIndex];

		// Reset whole pool per-frame (simple and correct for 1 primary cmd buffer per frame)
		Utilities::VulkanUtilities::VKCheck(vkResetCommandPool(m_Device, l_Frame.CommandPool, 0), "Failed vkResetCommandPool");

		VkCommandBufferBeginInfo l_BeginInfo{};
		l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		l_BeginInfo.flags = usageFlags;
		l_BeginInfo.pInheritanceInfo = nullptr;

		Utilities::VulkanUtilities::VKCheck(vkBeginCommandBuffer(l_Frame.PrimaryCommandBuffer, &l_BeginInfo), "Failed vkBeginCommandBuffer");

		return l_Frame.PrimaryCommandBuffer;
	}

	void VulkanCommand::EndFrame(uint32_t frameIndex)
	{
		if (frameIndex >= m_Frames.size())
		{
			TR_CORE_CRITICAL("VulkanCommand::EndFrame invalid frameIndex {}", frameIndex);

			std::abort();
		}

		PerFrameCommand& l_Frame = m_Frames[frameIndex];

		Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(l_Frame.PrimaryCommandBuffer), "Failed vkEndCommandBuffer");
	}

	VkCommandPool VulkanCommand::GetCommandPool(uint32_t frameIndex) const
	{
		if (frameIndex >= m_Frames.size())
		{
			TR_CORE_CRITICAL("VulkanCommand::GetCommandPool invalid frameIndex {}", frameIndex);

			std::abort();
		}

		return m_Frames[frameIndex].CommandPool;
	}

	VkCommandBuffer VulkanCommand::GetCommandBuffer(uint32_t frameIndex) const
	{
		if (frameIndex >= m_Frames.size())
		{
			TR_CORE_CRITICAL("VulkanCommand::GetCommandBuffer invalid frameIndex {}", frameIndex);

			std::abort();
		}

		return m_Frames[frameIndex].PrimaryCommandBuffer;
	}

	VkCommandBuffer VulkanCommand::BeginSingleTime(VkCommandPool commandPool) const
	{
		VkCommandBufferAllocateInfo l_AllocInfo{};
		l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		l_AllocInfo.commandPool = commandPool;
		l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		l_AllocInfo.commandBufferCount = 1;

		VkCommandBuffer l_CommandBuffer = VK_NULL_HANDLE;
		Utilities::VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_Device, &l_AllocInfo, &l_CommandBuffer), "Failed vkAllocateCommandBuffers (single-time)");

		VkCommandBufferBeginInfo l_BeginInfo{};
		l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		Utilities::VulkanUtilities::VKCheck(vkBeginCommandBuffer(l_CommandBuffer, &l_BeginInfo), "Failed vkBeginCommandBuffer (single-time)");

		return l_CommandBuffer;
	}

	void VulkanCommand::EndSingleTime(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue) const
	{
		Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(commandBuffer), "Failed vkEndCommandBuffer (single-time)");

		VkSubmitInfo l_SubmitInfo{};
		l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		l_SubmitInfo.commandBufferCount = 1;
		l_SubmitInfo.pCommandBuffers = &commandBuffer;

		// Fence-based wait avoids nuking the whole queue with vkQueueWaitIdle.
		VkFenceCreateInfo l_FenceInfo{};
		l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VkFence l_Fence = VK_NULL_HANDLE;
		Utilities::VulkanUtilities::VKCheck(vkCreateFence(m_Device, &l_FenceInfo, m_Allocator, &l_Fence), "Failed vkCreateFence (single-time)");
		Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(queue, 1, &l_SubmitInfo, l_Fence), "Failed vkQueueSubmit (single-time)");
		Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &l_Fence, VK_TRUE, std::numeric_limits<uint64_t>::max()), "Failed vkWaitForFences (single-time)");

		vkDestroyFence(m_Device, l_Fence, m_Allocator);

		vkFreeCommandBuffers(m_Device, commandPool, 1, &commandBuffer);
	}

	void VulkanCommand::CreatePerFrame(uint32_t framesInFlight)
	{
		m_Frames.clear();
		m_Frames.resize(framesInFlight);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			PerFrameCommand& l_Frame = m_Frames[i];

			VkCommandPoolCreateInfo l_PoolInfo{};
			l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			l_PoolInfo.queueFamilyIndex = m_GraphicsQueueFamilyIndex;

			Utilities::VulkanUtilities::VKCheck(vkCreateCommandPool(m_Device, &l_PoolInfo, m_Allocator, &l_Frame.CommandPool), "Failed vkCreateCommandPool (per-frame)");

			VkCommandBufferAllocateInfo l_AllocInfo{};
			l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			l_AllocInfo.commandPool = l_Frame.CommandPool;
			l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			l_AllocInfo.commandBufferCount = 1;

			Utilities::VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_Device, &l_AllocInfo, &l_Frame.PrimaryCommandBuffer), "Failed vkAllocateCommandBuffers (per-frame)");
		}
	}

	void VulkanCommand::DestroyPerFrame()
	{
		for (PerFrameCommand& it_Frame : m_Frames)
		{
			if (it_Frame.CommandPool != VK_NULL_HANDLE)
			{
				// Command buffers are implicitly freed when destroying the pool.
				vkDestroyCommandPool(m_Device, it_Frame.CommandPool, m_Allocator);
			}

			it_Frame = {};
		}

		m_Frames.clear();
	}

	void VulkanCommand::CreateUploadPool()
	{
		if (m_UploadCommandPool != VK_NULL_HANDLE)
		{
			return;
		}

		VkCommandPoolCreateInfo l_PoolInfo{};
		l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		l_PoolInfo.queueFamilyIndex = m_TransferQueueFamilyIndex;

		Utilities::VulkanUtilities::VKCheck(vkCreateCommandPool(m_Device, &l_PoolInfo, m_Allocator, &m_UploadCommandPool), "Failed vkCreateCommandPool (upload)");
	}

	void VulkanCommand::DestroyUploadPool()
	{
		if (m_UploadCommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(m_Device, m_UploadCommandPool, m_Allocator);
			m_UploadCommandPool = VK_NULL_HANDLE;
		}
	}
}