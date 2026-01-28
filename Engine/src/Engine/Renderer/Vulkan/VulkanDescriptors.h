#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace Engine
{
    class VulkanDescriptors
    {
    public:
        void Initialize(VkDevice device);
        void Shutdown();

        void CreateLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
        void DestroyLayout();

        void CreatePool(uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes);
        void DestroyPool();

        void AllocateSets(uint32_t setCount, std::vector<VkDescriptorSet>& outSets) const;

        void UpdateBuffer(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkBuffer buffer, VkDeviceSize range, VkDeviceSize offset = 0) const;
        void UpdateImage(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkImageView imageView, VkSampler sampler,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;

        VkDescriptorSetLayout GetLayout() const { return m_Layout; }
        VkDescriptorPool GetPool() const { return m_Pool; }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;

        VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
        VkDescriptorPool m_Pool = VK_NULL_HANDLE;
    };
}