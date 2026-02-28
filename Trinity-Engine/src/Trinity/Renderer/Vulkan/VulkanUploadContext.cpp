#include "Trinity/Renderer/Vulkan/VulkanUploadContext.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>

namespace Trinity
{
	void VulkanUploadContext::Initialize(const VulkanContext& context, const VulkanDevice& device)
	{
		if (m_Device != VK_NULL_HANDLE)
		{
			TR_CORE_WARN("VulkanUploadContext::Initialize called while already initialized. Reinitializing.");
			Shutdown();
		}

		m_Allocator = context.GetAllocator();
		m_Device = device.GetDevice();
		m_Queue = device.GetGraphicsQueue();

		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanUploadContext::Initialize - invalid VkDevice");
			std::abort();
		}

		VkCommandPoolCreateInfo l_PoolInfo{};
		l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		l_PoolInfo.queueFamilyIndex = device.GetGraphicsQueueFamilyIndex();
		Utilities::VulkanUtilities::VKCheck(vkCreateCommandPool(m_Device, &l_PoolInfo, m_Allocator, &m_Pool), "VulkanUploadContext: vkCreateCommandPool failed");

		VkCommandBufferAllocateInfo l_AllocInfo{};
		l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		l_AllocInfo.commandPool = m_Pool;
		l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		l_AllocInfo.commandBufferCount = 1;
		Utilities::VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_Device, &l_AllocInfo, &m_CommandBuffer), "VulkanUploadContext: vkAllocateCommandBuffers failed");

		VkFenceCreateInfo l_FenceInfo{};
		l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		l_FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		Utilities::VulkanUtilities::VKCheck(vkCreateFence(m_Device, &l_FenceInfo, m_Allocator, &m_Fence), "VulkanUploadContext: vkCreateFence failed");

		TR_CORE_TRACE("VulkanUploadContext Initialized");
	}

	void VulkanUploadContext::Shutdown()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		if (m_Fence != VK_NULL_HANDLE)
		{
			vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);
			vkDestroyFence(m_Device, m_Fence, m_Allocator);
			m_Fence = VK_NULL_HANDLE;
		}

		if (m_Pool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(m_Device, m_Pool, m_Allocator);
			m_Pool = VK_NULL_HANDLE;
			m_CommandBuffer = VK_NULL_HANDLE;
		}

		m_Queue = VK_NULL_HANDLE;
		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;

		TR_CORE_TRACE("VulkanUploadContext Shutdown");
	}

	void VulkanUploadContext::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size,
		VkDeviceSize srcOffset, VkDeviceSize dstOffset)
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanUploadContext::CopyBuffer called before Initialize");
			std::abort();
		}

		Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX), "VulkanUploadContext: vkWaitForFences failed");
		Utilities::VulkanUtilities::VKCheck(vkResetFences(m_Device, 1, &m_Fence), "VulkanUploadContext: vkResetFences failed");
		Utilities::VulkanUtilities::VKCheck(vkResetCommandPool(m_Device, m_Pool, 0), "VulkanUploadContext: vkResetCommandPool failed");

		VkCommandBufferBeginInfo l_BeginInfo{};
		l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		Utilities::VulkanUtilities::VKCheck(vkBeginCommandBuffer(m_CommandBuffer, &l_BeginInfo), "VulkanUploadContext: vkBeginCommandBuffer failed");

		VkBufferCopy l_Region{};
		l_Region.srcOffset = srcOffset;
		l_Region.dstOffset = dstOffset;
		l_Region.size = size;
		vkCmdCopyBuffer(m_CommandBuffer, src, dst, 1, &l_Region);

		Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(m_CommandBuffer), "VulkanUploadContext: vkEndCommandBuffer failed");

		VkSubmitInfo l_Submit{};
		l_Submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		l_Submit.commandBufferCount = 1;
		l_Submit.pCommandBuffers = &m_CommandBuffer;
		Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_Queue, 1, &l_Submit, m_Fence), "VulkanUploadContext: vkQueueSubmit failed");
		Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX), "VulkanUploadContext: vkWaitForFences (post-submit) failed");
	}
}