#pragma once

#include "Trinity/Renderer/Resources/DescriptorSet.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace Trinity
{
    class VulkanDescriptorAllocator;

    class VulkanDescriptorSet final : public DescriptorSet
    {
    public:
        VulkanDescriptorSet(VkDevice device, VkDescriptorSet set, std::shared_ptr<DescriptorSetLayout> layout, VulkanDescriptorAllocator* allocator, bool isPersistent);
        ~VulkanDescriptorSet() override;

        void WriteUniformBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range) override;
        void WriteStorageBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range) override;
        void WriteSampledImage(uint32_t binding, const std::shared_ptr<Texture>& texture, const std::shared_ptr<Sampler>& sampler) override;
        void WriteStorageImage(uint32_t binding, const std::shared_ptr<Texture>& texture) override;
        void WriteSampler(uint32_t binding, const std::shared_ptr<Sampler>& sampler) override;

        void WriteUniformBufferArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range) override;
        void WriteStorageBufferArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range) override;
        void WriteSampledImageArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Texture>& texture, const std::shared_ptr<Sampler>& sampler) override;
        void WriteStorageImageArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Texture>& texture) override;

        void Flush() override;

        std::shared_ptr<DescriptorSetLayout> GetLayout() const override { return m_Layout; }

        VkDescriptorSet GetVkSet() const { return m_Set; }

    private:
        struct PendingWrite
        {
            uint32_t Binding = 0;
            uint32_t ArrayElement = 0;
            VkDescriptorType Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            VkDescriptorBufferInfo Buffer{};
            VkDescriptorImageInfo Image{};
            bool IsBuffer = false;
        };

        void EnqueueBufferWrite(uint32_t binding, uint32_t arrayElement, VkDescriptorType type, VkBuffer buffer, uint64_t offset, uint64_t range);
        void EnqueueImageWrite(uint32_t binding, uint32_t arrayElement, VkDescriptorType type, VkImageView view, VkSampler sampler, VkImageLayout layout);

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkDescriptorSet m_Set = VK_NULL_HANDLE;
        std::shared_ptr<DescriptorSetLayout> m_Layout;
        VulkanDescriptorAllocator* m_Allocator = nullptr;
        bool m_IsPersistent = false;

        std::vector<PendingWrite> m_Pending;
    };
}