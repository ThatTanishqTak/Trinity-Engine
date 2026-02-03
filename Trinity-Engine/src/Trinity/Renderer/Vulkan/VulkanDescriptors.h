#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class VulkanDevice;

	class VulkanDescriptors
	{
	public:
		struct PerFrameDescriptorSets
		{
			VkDescriptorSet GlobalSet = VK_NULL_HANDLE;
		};

	public:
		VulkanDescriptors() = default;
		~VulkanDescriptors() = default;

		VulkanDescriptors(const VulkanDescriptors&) = delete;
		VulkanDescriptors& operator=(const VulkanDescriptors&) = delete;
		VulkanDescriptors(VulkanDescriptors&&) = delete;
		VulkanDescriptors& operator=(VulkanDescriptors&&) = delete;

		void Initialize(const VulkanDevice& device, uint32_t framesInFlight, VkAllocationCallbacks* allocator = nullptr);
		void Shutdown();

		VkDescriptorSetLayout GetGlobalSetLayout() const { return m_GlobalSetLayout; }
		VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }

		uint32_t GetFrameCount() const { return static_cast<uint32_t>(m_Frames.size()); }
		VkDescriptorSet GetGlobalSet(uint32_t frameIndex) const;

		void WriteGlobalUBO(uint32_t frameIndex, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);
		void WriteGlobalTexture(uint32_t frameIndex, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		void WriteUniformBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);
		void WriteCombinedImageSampler(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		void WriteStorageBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

	private:
		void CreateLayouts();
		void DestroyLayouts();

		void CreateDescriptorPool(uint32_t maxSets, uint32_t uniformBufferCount, uint32_t combinedImageSamplerCount, uint32_t storageBufferCount);
		void DestroyDescriptorPool();

		void AllocatePerFrameDescriptorSets(uint32_t framesInFlight);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkAllocationCallbacks* m_Allocator = nullptr;

		VkDescriptorSetLayout m_GlobalSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

		std::vector<PerFrameDescriptorSets> m_Frames;
	};
}