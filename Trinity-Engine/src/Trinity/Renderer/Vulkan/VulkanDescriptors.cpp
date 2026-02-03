#include "Trinity/Renderer/Vulkan/VulkanDescriptors.h"

#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Utilities/Utilities.h"

#include <cstdlib>

namespace Trinity
{
	void VulkanDescriptors::Initialize(const VulkanDevice& device, uint32_t framesInFlight, VkAllocationCallbacks* allocator)
	{
		if (framesInFlight == 0)
		{
			TR_CORE_CRITICAL("VulkanDescriptors::Initialize called with framesInFlight == 0");

			std::abort();
		}

		m_Device = device.GetDevice();
		m_Allocator = allocator;

		if (m_Device == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanDescriptors::Initialize called with invalid VkDevice");

			std::abort();
		}

		CreateLayouts();

		const uint32_t l_MaxSets = framesInFlight + 32;

		// binding 0: UBO (1 per set)
		// binding 1: Combined image sampler (1 per set) - optional at runtime, but reserve capacity anyway
		const uint32_t l_UBOCount = l_MaxSets * 1;
		const uint32_t l_SamplerCount = l_MaxSets * 1;

		const uint32_t l_StorageCount = 0;

		CreateDescriptorPool(l_MaxSets, l_UBOCount, l_SamplerCount, l_StorageCount);
		AllocatePerFrameDescriptorSets(framesInFlight);

		TR_CORE_TRACE("VulkanDescriptors initialized (framesInFlight = {})", framesInFlight);
	}

	void VulkanDescriptors::Shutdown()
	{
		if (m_Device == VK_NULL_HANDLE)
		{
			m_Frames.clear();

			return;
		}

		m_Frames.clear();

		DestroyDescriptorPool();
		DestroyLayouts();

		m_Device = VK_NULL_HANDLE;
		m_Allocator = nullptr;
	}

	VkDescriptorSet VulkanDescriptors::GetGlobalSet(uint32_t frameIndex) const
	{
		if (frameIndex >= m_Frames.size())
		{
			TR_CORE_CRITICAL("VulkanDescriptors::GetGlobalSet invalid frameIndex {}", frameIndex);

			std::abort();
		}

		return m_Frames[frameIndex].GlobalSet;
	}

	void VulkanDescriptors::WriteGlobalUBO(uint32_t frameIndex, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		if (buffer == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanDescriptors::WriteGlobalUBO called with VK_NULL_HANDLE buffer");

			std::abort();
		}

		VkDescriptorSet l_Set = GetGlobalSet(frameIndex);
		WriteUniformBuffer(l_Set, 0, buffer, offset, range);
	}

	void VulkanDescriptors::WriteGlobalTexture(uint32_t frameIndex, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout)
	{
		if (imageView == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE)
		{
			TR_CORE_CRITICAL("VulkanDescriptors::WriteGlobalTexture called with invalid imageView/sampler");

			std::abort();
		}

		VkDescriptorSet l_Set = GetGlobalSet(frameIndex);
		WriteCombinedImageSampler(l_Set, 1, imageView, sampler, imageLayout);
	}

	void VulkanDescriptors::WriteUniformBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		VkDescriptorBufferInfo l_BufferInfo{};
		l_BufferInfo.buffer = buffer;
		l_BufferInfo.offset = offset;
		l_BufferInfo.range = range;

		VkWriteDescriptorSet l_Write{};
		l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		l_Write.dstSet = descriptorSet;
		l_Write.dstBinding = binding;
		l_Write.dstArrayElement = 0;
		l_Write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		l_Write.descriptorCount = 1;
		l_Write.pBufferInfo = &l_BufferInfo;

		vkUpdateDescriptorSets(m_Device, 1, &l_Write, 0, nullptr);
	}

	void VulkanDescriptors::WriteCombinedImageSampler(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout)
	{
		VkDescriptorImageInfo l_ImageInfo{};
		l_ImageInfo.sampler = sampler;
		l_ImageInfo.imageView = imageView;
		l_ImageInfo.imageLayout = imageLayout;

		VkWriteDescriptorSet l_Write{};
		l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		l_Write.dstSet = descriptorSet;
		l_Write.dstBinding = binding;
		l_Write.dstArrayElement = 0;
		l_Write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		l_Write.descriptorCount = 1;
		l_Write.pImageInfo = &l_ImageInfo;

		vkUpdateDescriptorSets(m_Device, 1, &l_Write, 0, nullptr);
	}

	void VulkanDescriptors::WriteStorageBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		VkDescriptorBufferInfo l_BufferInfo{};
		l_BufferInfo.buffer = buffer;
		l_BufferInfo.offset = offset;
		l_BufferInfo.range = range;

		VkWriteDescriptorSet l_Write{};
		l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		l_Write.dstSet = descriptorSet;
		l_Write.dstBinding = binding;
		l_Write.dstArrayElement = 0;
		l_Write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		l_Write.descriptorCount = 1;
		l_Write.pBufferInfo = &l_BufferInfo;

		vkUpdateDescriptorSets(m_Device, 1, &l_Write, 0, nullptr);
	}

	void VulkanDescriptors::CreateLayouts()
	{
		// Global layout (Set 0):
		// binding 0: Uniform buffer (camera/scene)
		// binding 1: Combined image sampler (optional default texture slot)
		VkDescriptorSetLayoutBinding l_UBOBinding{};
		l_UBOBinding.binding = 0;
		l_UBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		l_UBOBinding.descriptorCount = 1;
		l_UBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		l_UBOBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding l_SamplerBinding{};
		l_SamplerBinding.binding = 1;
		l_SamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		l_SamplerBinding.descriptorCount = 1;
		l_SamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		l_SamplerBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding l_Bindings[] = { l_UBOBinding, l_SamplerBinding };

		VkDescriptorSetLayoutCreateInfo l_LayoutInfo{};
		l_LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		l_LayoutInfo.bindingCount = static_cast<uint32_t>(sizeof(l_Bindings) / sizeof(l_Bindings[0]));
		l_LayoutInfo.pBindings = l_Bindings;

		Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device, &l_LayoutInfo, m_Allocator, &m_GlobalSetLayout), "Failed vkCreateDescriptorSetLayout (Global)");
	}

	void VulkanDescriptors::DestroyLayouts()
	{
		if (m_GlobalSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_Device, m_GlobalSetLayout, m_Allocator);
			m_GlobalSetLayout = VK_NULL_HANDLE;
		}
	}

	void VulkanDescriptors::CreateDescriptorPool(uint32_t maxSets, uint32_t uniformBufferCount, uint32_t combinedImageSamplerCount, uint32_t storageBufferCount)
	{
		if (maxSets == 0)
		{
			TR_CORE_CRITICAL("VulkanDescriptors::CreateDescriptorPool called with maxSets == 0");

			std::abort();
		}

		std::vector<VkDescriptorPoolSize> l_PoolSizes;
		l_PoolSizes.reserve(3);

		if (uniformBufferCount > 0)
		{
			VkDescriptorPoolSize l_Size{};
			l_Size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			l_Size.descriptorCount = uniformBufferCount;
			l_PoolSizes.push_back(l_Size);
		}

		if (combinedImageSamplerCount > 0)
		{
			VkDescriptorPoolSize l_Size{};
			l_Size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			l_Size.descriptorCount = combinedImageSamplerCount;
			l_PoolSizes.push_back(l_Size);
		}

		if (storageBufferCount > 0)
		{
			VkDescriptorPoolSize l_Size{};
			l_Size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			l_Size.descriptorCount = storageBufferCount;
			l_PoolSizes.push_back(l_Size);
		}

		if (l_PoolSizes.empty())
		{
			TR_CORE_CRITICAL("VulkanDescriptors::CreateDescriptorPool has no pool sizes (all counts were 0)");

			std::abort();
		}

		VkDescriptorPoolCreateInfo l_PoolInfo{};
		l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		l_PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		l_PoolInfo.maxSets = maxSets;
		l_PoolInfo.poolSizeCount = static_cast<uint32_t>(l_PoolSizes.size());
		l_PoolInfo.pPoolSizes = l_PoolSizes.data();

		Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Device, &l_PoolInfo, m_Allocator, &m_DescriptorPool), "Failed vkCreateDescriptorPool");
	}

	void VulkanDescriptors::DestroyDescriptorPool()
	{
		if (m_DescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_Device, m_DescriptorPool, m_Allocator);
			m_DescriptorPool = VK_NULL_HANDLE;
		}
	}

	void VulkanDescriptors::AllocatePerFrameDescriptorSets(uint32_t framesInFlight)
	{
		m_Frames.clear();
		m_Frames.resize(framesInFlight);

		std::vector<VkDescriptorSetLayout> l_Layouts(framesInFlight, m_GlobalSetLayout);
		std::vector<VkDescriptorSet> l_Sets(framesInFlight, VK_NULL_HANDLE);

		VkDescriptorSetAllocateInfo l_AllocInfo{};
		l_AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		l_AllocInfo.descriptorPool = m_DescriptorPool;
		l_AllocInfo.descriptorSetCount = static_cast<uint32_t>(l_Layouts.size());
		l_AllocInfo.pSetLayouts = l_Layouts.data();

		Utilities::VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Device, &l_AllocInfo, l_Sets.data()), "Failed vkAllocateDescriptorSets (per-frame Global)");

		for (uint32_t l_i = 0; l_i < framesInFlight; ++l_i)
		{
			m_Frames[l_i].GlobalSet = l_Sets[l_i];
		}
	}
}