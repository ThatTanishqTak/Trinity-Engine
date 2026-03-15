#include "Trinity/Renderer/Vulkan/VulkanDescriptorAllocator.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <cstdlib>
#include <array>

namespace Trinity
{
	void VulkanDescriptorAllocator::Initialize(VkDevice device, VkAllocationCallbacks* allocator, uint32_t framesInFlight, uint32_t maxSetsPerFrame)
	{
		if (m_Device != VK_NULL_HANDLE)
		{
			TR_CORE_WARN("VulkanDescriptorAllocator::Initialize called while already initialized. Reinitializing.");

			Shutdown();
		}

		if (framesInFlight == 0 || maxSetsPerFrame == 0)
		{
			TR_CORE_CRITICAL("VulkanDescriptorAllocator::Initialize — framesInFlight and maxSetsPerFrame must be > 0");

			std::abort();
		}

		m_Device = device;
		m_HostAllocator = allocator;
		m_Pools.resize(framesInFlight, VK_NULL_HANDLE);

		const std::array<VkDescriptorPoolSize, 1> l_PoolSizes = { { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxSetsPerFrame } } };

		VkDescriptorPoolCreateInfo l_PoolInfo{};
		l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		l_PoolInfo.maxSets = maxSetsPerFrame;
		l_PoolInfo.poolSizeCount = static_cast<uint32_t>(l_PoolSizes.size());
		l_PoolInfo.pPoolSizes = l_PoolSizes.data();

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Device, &l_PoolInfo, m_HostAllocator, &m_Pools[i]), "VulkanDescriptorAllocator: vkCreateDescriptorPool failed");
		}

		TR_CORE_TRACE("VulkanDescriptorAllocator Initialized ({} frames, {} sets/frame)", framesInFlight, maxSetsPerFrame);
	}

	void VulkanDescriptorAllocator::Shutdown()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		for (VkDescriptorPool& it_Pool : m_Pools)
		{
			if (it_Pool != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorPool(m_Device, it_Pool, m_HostAllocator);
				it_Pool = VK_NULL_HANDLE;
			}
		}

		m_Pools.clear();
		m_Device = VK_NULL_HANDLE;
		m_HostAllocator = nullptr;

		TR_CORE_TRACE("VulkanDescriptorAllocator Shutdown");
	}

	void VulkanDescriptorAllocator::BeginFrame(uint32_t frameIndex)
	{
		if (frameIndex >= static_cast<uint32_t>(m_Pools.size()))
		{
			TR_CORE_CRITICAL("VulkanDescriptorAllocator::BeginFrame — frameIndex {} out of range", frameIndex);

			std::abort();
		}

		Utilities::VulkanUtilities::VKCheck(vkResetDescriptorPool(m_Device, m_Pools[frameIndex], 0), "VulkanDescriptorAllocator: vkResetDescriptorPool failed");
	}

	VkDescriptorSet VulkanDescriptorAllocator::Allocate(uint32_t frameIndex, VkDescriptorSetLayout layout)
	{
		if (frameIndex >= static_cast<uint32_t>(m_Pools.size()))
		{
			TR_CORE_CRITICAL("VulkanDescriptorAllocator::Allocate — frameIndex {} out of range", frameIndex);

			std::abort();
		}

		VkDescriptorSetAllocateInfo l_AllocInfo{};
		l_AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		l_AllocInfo.descriptorPool = m_Pools[frameIndex];
		l_AllocInfo.descriptorSetCount = 1;
		l_AllocInfo.pSetLayouts = &layout;

		VkDescriptorSet l_Set = VK_NULL_HANDLE;
		Utilities::VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Device, &l_AllocInfo, &l_Set), "VulkanDescriptorAllocator: vkAllocateDescriptorSets failed");

		return l_Set;
	}
}