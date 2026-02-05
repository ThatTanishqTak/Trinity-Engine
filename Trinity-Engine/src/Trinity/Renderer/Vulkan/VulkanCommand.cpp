#include "Trinity/Renderer/Vulkan/VulkanCommand.h"

#include "Trinity/Utilities/Utilities.h"

#include <cstdlib>
#include <limits>
#include <utility>

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
		m_TransferQueueFamilyIndex = context.Queues.TransferFamilyIndex;
		m_UploadQueue = context.Queues.TransferQueue;

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
		m_UploadFence = VK_NULL_HANDLE;
		m_UploadCommandBufferInFlight = VK_NULL_HANDLE;
		m_UploadBarriers.clear();
		m_UploadWaits.clear();
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

	VkCommandBuffer VulkanCommand::BeginSingleTime(VkCommandPool commandPool)
	{
		if (m_UploadFence != VK_NULL_HANDLE && m_UploadCommandBufferInFlight != VK_NULL_HANDLE)
		{
			Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &m_UploadFence, VK_TRUE, std::numeric_limits<uint64_t>::max()), "Failed vkWaitForFences (upload)");
			vkFreeCommandBuffers(m_Device, commandPool, 1, &m_UploadCommandBufferInFlight);
			m_UploadCommandBufferInFlight = VK_NULL_HANDLE;
		}
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

	void VulkanCommand::EndSingleTime(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue, VkPipelineStageFlags waitStageMask)
	{
		Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(commandBuffer), "Failed vkEndCommandBuffer (single-time)");

		VkSubmitInfo l_SubmitInfo{};
		l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		l_SubmitInfo.commandBufferCount = 1;
		l_SubmitInfo.pCommandBuffers = &commandBuffer;

		VkSemaphoreCreateInfo l_SemaphoreInfo{};
		l_SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkSemaphore l_Semaphore = VK_NULL_HANDLE;
		Utilities::VulkanUtilities::VKCheck(vkCreateSemaphore(m_Device, &l_SemaphoreInfo, m_Allocator, &l_Semaphore), "Failed vkCreateSemaphore (upload)");

		l_SubmitInfo.signalSemaphoreCount = 1;
		l_SubmitInfo.pSignalSemaphores = &l_Semaphore;

		if (m_UploadFence != VK_NULL_HANDLE)
		{
			Utilities::VulkanUtilities::VKCheck(vkResetFences(m_Device, 1, &m_UploadFence), "Failed vkResetFences (upload)");
		}

		Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(queue, 1, &l_SubmitInfo, m_UploadFence), "Failed vkQueueSubmit (single-time)");

		m_UploadCommandBufferInFlight = commandBuffer;

		UploadWait l_Wait{};
		l_Wait.Semaphore = l_Semaphore;
		l_Wait.StageMask = waitStageMask != 0 ? waitStageMask : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		m_UploadWaits.push_back(l_Wait);
	}

	void VulkanCommand::EnqueueUploadBarrier(const UploadBarrier& barrier)
	{
		m_UploadBarriers.push_back(barrier);
	}

	void VulkanCommand::RecordUploadAcquireBarriers(VkCommandBuffer commandBuffer)
	{
		if (m_UploadBarriers.empty())
		{
			return;
		}

		std::vector<VkBufferMemoryBarrier> l_Barriers;
		l_Barriers.reserve(m_UploadBarriers.size());

		for (const UploadBarrier& it_Barrier : m_UploadBarriers)
		{
			VkBufferMemoryBarrier l_BufferBarrier{};
			l_BufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			l_BufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			l_BufferBarrier.dstAccessMask = it_Barrier.DstAccessMask;
			l_BufferBarrier.srcQueueFamilyIndex = m_TransferQueueFamilyIndex;
			l_BufferBarrier.dstQueueFamilyIndex = m_GraphicsQueueFamilyIndex;
			l_BufferBarrier.buffer = it_Barrier.Buffer;
			l_BufferBarrier.offset = it_Barrier.Offset;
			l_BufferBarrier.size = it_Barrier.Size;
			l_Barriers.push_back(l_BufferBarrier);
		}

		VkPipelineStageFlags l_DstStageMask = 0;

		for (const UploadBarrier& it_Barrier : m_UploadBarriers)
		{
			l_DstStageMask |= it_Barrier.DstStageMask;
		}

		if (l_DstStageMask == 0)
		{
			l_DstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		}

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, l_DstStageMask, 0, 0, nullptr, static_cast<uint32_t>(l_Barriers.size()), l_Barriers.data(), 0, nullptr);

		m_UploadBarriers.clear();
	}

	void VulkanCommand::ConsumeUploadWaits(std::vector<UploadWait>& waits)
	{
		waits = std::move(m_UploadWaits);
		m_UploadWaits.clear();
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

		VkFenceCreateInfo l_FenceInfo{};
		l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		Utilities::VulkanUtilities::VKCheck(vkCreateFence(m_Device, &l_FenceInfo, m_Allocator, &m_UploadFence), "Failed vkCreateFence (upload)");
	}

	void VulkanCommand::DestroyUploadPool()
	{
		if (m_UploadFence != VK_NULL_HANDLE)
		{
			Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &m_UploadFence, VK_TRUE, std::numeric_limits<uint64_t>::max()), "Failed vkWaitForFences (upload)");
		}

		if (m_UploadCommandBufferInFlight != VK_NULL_HANDLE && m_UploadCommandPool != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(m_Device, m_UploadCommandPool, 1, &m_UploadCommandBufferInFlight);
			m_UploadCommandBufferInFlight = VK_NULL_HANDLE;
		}

		if (m_UploadFence != VK_NULL_HANDLE)
		{
			vkDestroyFence(m_Device, m_UploadFence, m_Allocator);
			m_UploadFence = VK_NULL_HANDLE;
		}

		m_UploadBarriers.clear();
		for (UploadWait& it_Wait : m_UploadWaits)
		{
			if (it_Wait.Semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device, it_Wait.Semaphore, m_Allocator);
				it_Wait.Semaphore = VK_NULL_HANDLE;
			}
		}

		m_UploadWaits.clear();

		if (m_UploadCommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(m_Device, m_UploadCommandPool, m_Allocator);
			m_UploadCommandPool = VK_NULL_HANDLE;
		}
	}
}