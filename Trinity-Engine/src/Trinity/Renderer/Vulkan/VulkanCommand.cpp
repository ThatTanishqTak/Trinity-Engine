#include "Trinity/Renderer/Vulkan/VulkanCommand.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>

namespace Trinity
{
	void VulkanCommand::Initialize(const VulkanContext& context, const VulkanDevice& device, uint32_t framesInFlight)
	{
		TR_CORE_TRACE("Initializing Vulkan Command");

		if (m_Device != VK_NULL_HANDLE)
		{
			TR_CORE_WARN("VulkanCommand::Initialize called while already initialized Reinitializing");

			Shutdown();
		}

		const VkDevice l_Device = device.GetDevice();
		if (l_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanCommand::Initialize called with invalid VkDevice");

			std::abort();
		}

		if (framesInFlight == 0)
		{
			TR_CORE_CRITICAL("VulkanCommand::Initialize called with framesInFlight = 0");

			std::abort();
		}

		m_Device = l_Device;
		m_Allocator = context.GetAllocator();
		m_GraphicsQueueFamilyIndex = device.GetGraphicsQueueFamilyIndex();
		m_FramesInFlight = framesInFlight;
		m_CommandPools.resize(m_FramesInFlight, VK_NULL_HANDLE);
		m_CommandBuffers.resize(m_FramesInFlight, VK_NULL_HANDLE);

		for (uint32_t l_FrameIndex = 0; l_FrameIndex < m_FramesInFlight; ++l_FrameIndex)
		{
			VkCommandPoolCreateInfo l_PoolInfo{};
			l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			l_PoolInfo.queueFamilyIndex = m_GraphicsQueueFamilyIndex;

			Utilities::VulkanUtilities::VKCheck(vkCreateCommandPool(m_Device, &l_PoolInfo, m_Allocator, &m_CommandPools[l_FrameIndex]), "Failed vkCreateCommandPool");

			VkCommandBufferAllocateInfo l_AllocInfo{};
			l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			l_AllocInfo.commandPool = m_CommandPools[l_FrameIndex];
			l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			l_AllocInfo.commandBufferCount = 1;

			Utilities::VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_Device, &l_AllocInfo, &m_CommandBuffers[l_FrameIndex]), "Failed vkAllocateCommandBuffers");
		}

		TR_CORE_TRACE("Vulkan Command Initialized (FramesInFlight: {})", m_FramesInFlight);
	}

	void VulkanCommand::Shutdown()
	{
		TR_CORE_TRACE("Shutting Down Vulkan Command");

		DestroyCommandObjects();

		m_CommandPools.clear();
		m_CommandBuffers.clear();
		m_CommandPools.shrink_to_fit();
		m_CommandBuffers.shrink_to_fit();
		m_Allocator = nullptr;
		m_Device = VK_NULL_HANDLE;
		m_GraphicsQueueFamilyIndex = 0;
		m_FramesInFlight = 0;

		TR_CORE_TRACE("Vulkan Command Shutdown Complete");
	}

	VkCommandPool VulkanCommand::GetCommandPool(uint32_t frameIndex) const
	{
		ValidateFrameIndex(frameIndex);

		return m_CommandPools[frameIndex];
	}

	VkCommandBuffer VulkanCommand::GetCommandBuffer(uint32_t frameIndex) const
	{
		ValidateFrameIndex(frameIndex);

		return m_CommandBuffers[frameIndex];
	}

	void VulkanCommand::Reset(uint32_t frameIndex) const
	{
		ValidateFrameIndex(frameIndex);

		const VkCommandPool l_Pool = m_CommandPools[frameIndex];
		if (l_Pool == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanCommand::Reset called with null command pool");

			std::abort();
		}

		Utilities::VulkanUtilities::VKCheck(vkResetCommandPool(m_Device, l_Pool, 0), "Failed vkResetCommandPool");
	}

	void VulkanCommand::Begin(uint32_t frameIndex) const
	{
		ValidateFrameIndex(frameIndex);

		const VkCommandBuffer l_CommandBuffer = m_CommandBuffers[frameIndex];
		if (l_CommandBuffer == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanCommand::Begin called with null command buffer");

			std::abort();
		}

		VkCommandBufferBeginInfo l_BeginInfo{};
		l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		Utilities::VulkanUtilities::VKCheck(vkBeginCommandBuffer(l_CommandBuffer, &l_BeginInfo), "Failed vkBeginCommandBuffer");
	}

	void VulkanCommand::End(uint32_t frameIndex) const
	{
		ValidateFrameIndex(frameIndex);

		const VkCommandBuffer l_CommandBuffer = m_CommandBuffers[frameIndex];
		if (l_CommandBuffer == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanCommand::End called with null command buffer");

			std::abort();
		}

		Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(l_CommandBuffer), "Failed vkEndCommandBuffer");
	}

	void VulkanCommand::DestroyCommandObjects()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		for (VkCommandPool it_Pool : m_CommandPools)
		{
			if (it_Pool != VK_NULL_HANDLE)
			{
				vkDestroyCommandPool(m_Device, it_Pool, m_Allocator);
			}
		}
	}

	void VulkanCommand::ValidateFrameIndex(uint32_t frameIndex) const
	{
		if (frameIndex >= m_FramesInFlight)
		{
			TR_CORE_CRITICAL("VulkanCommand frame index out of range ({} >= {})", frameIndex, m_FramesInFlight);

			std::abort();
		}
	}
}