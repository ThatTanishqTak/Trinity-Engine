#include "Trinity/Renderer/Vulkan/Resources/VulkanDescriptorSet.h"

#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanSampler.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Renderer/Vulkan/VulkanDescriptorAllocator.h"

namespace Trinity
{
    VulkanDescriptorSet::VulkanDescriptorSet(VkDevice device, VkDescriptorSet set, std::shared_ptr<DescriptorSetLayout> layout, VulkanDescriptorAllocator* allocator, bool isPersistent) : m_Device(device), m_Set(set), m_Layout(std::move(layout)), m_Allocator(allocator), m_IsPersistent(isPersistent)
    {

    }

    VulkanDescriptorSet::~VulkanDescriptorSet()
    {
        if (m_IsPersistent && m_Allocator != nullptr && m_Set != VK_NULL_HANDLE)
        {
            m_Allocator->FreePersistent(m_Set);
        }
    }

    void VulkanDescriptorSet::WriteUniformBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range)
    {
        WriteUniformBufferArray(binding, 0, buffer, offset, range);
    }

    void VulkanDescriptorSet::WriteStorageBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range)
    {
        WriteStorageBufferArray(binding, 0, buffer, offset, range);
    }

    void VulkanDescriptorSet::WriteSampledImage(uint32_t binding, const std::shared_ptr<Texture>& texture, const std::shared_ptr<Sampler>& sampler)
    {
        WriteSampledImageArray(binding, 0, texture, sampler);
    }

    void VulkanDescriptorSet::WriteStorageImage(uint32_t binding, const std::shared_ptr<Texture>& texture)
    {
        WriteStorageImageArray(binding, 0, texture);
    }

    void VulkanDescriptorSet::WriteSampler(uint32_t binding, const std::shared_ptr<Sampler>& sampler)
    {
        if (sampler == nullptr)
        {
            return;
        }

        auto* a_VulkanSampler = dynamic_cast<VulkanSampler*>(sampler.get());
        if (a_VulkanSampler == nullptr)
        {
            return;
        }

        EnqueueImageWrite(binding, 0, VK_DESCRIPTOR_TYPE_SAMPLER, VK_NULL_HANDLE, a_VulkanSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED);
    }

    void VulkanDescriptorSet::WriteUniformBufferArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range)
    {
        if (buffer == nullptr)
        {
            return;
        }

        auto* a_VulkanBuffer = dynamic_cast<VulkanBuffer*>(buffer.get());
        if (a_VulkanBuffer == nullptr)
        {
            return;
        }

        EnqueueBufferWrite(binding, arrayElement, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, a_VulkanBuffer->GetBuffer(), offset, range);
    }

    void VulkanDescriptorSet::WriteStorageBufferArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Buffer>& buffer, uint64_t offset, uint64_t range)
    {
        if (buffer == nullptr)
        {
            return;
        }

        auto* a_VulkanBuffer = dynamic_cast<VulkanBuffer*>(buffer.get());
        if (a_VulkanBuffer == nullptr)
        {
            return;
        }

        EnqueueBufferWrite(binding, arrayElement, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, a_VulkanBuffer->GetBuffer(), offset, range);
    }

    void VulkanDescriptorSet::WriteSampledImageArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Texture>& texture, const std::shared_ptr<Sampler>& sampler)
    {
        if (texture == nullptr)
        {
            return;
        }

        auto* a_VulkanTexture = dynamic_cast<VulkanTexture*>(texture.get());
        if (a_VulkanTexture == nullptr)
        {
            return;
        }

        VkSampler l_Sampler = VK_NULL_HANDLE;
        if (sampler != nullptr)
        {
            auto* a_VulkanSampler = dynamic_cast<VulkanSampler*>(sampler.get());
            if (a_VulkanSampler != nullptr)
            {
                l_Sampler = a_VulkanSampler->GetSampler();
            }
        }

        const VkDescriptorType l_Type = (l_Sampler != VK_NULL_HANDLE) ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        EnqueueImageWrite(binding, arrayElement, l_Type, a_VulkanTexture->GetImageView(), l_Sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void VulkanDescriptorSet::WriteStorageImageArray(uint32_t binding, uint32_t arrayElement, const std::shared_ptr<Texture>& texture)
    {
        if (texture == nullptr)
        {
            return;
        }

        auto* a_VulkanTexture = dynamic_cast<VulkanTexture*>(texture.get());
        if (a_VulkanTexture == nullptr)
        {
            return;
        }

        EnqueueImageWrite(binding, arrayElement, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, a_VulkanTexture->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL);
    }

    void VulkanDescriptorSet::Flush()
    {
        if (m_Pending.empty() || m_Set == VK_NULL_HANDLE)
        {
            return;
        }

        std::vector<VkWriteDescriptorSet> l_Writes;
        l_Writes.reserve(m_Pending.size());

        for (auto& it_Pending : m_Pending)
        {
            VkWriteDescriptorSet l_Write{};
            l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            l_Write.dstSet = m_Set;
            l_Write.dstBinding = it_Pending.Binding;
            l_Write.dstArrayElement = it_Pending.ArrayElement;
            l_Write.descriptorCount = 1;
            l_Write.descriptorType = it_Pending.Type;

            if (it_Pending.IsBuffer)
            {
                l_Write.pBufferInfo = &it_Pending.Buffer;
            }
            else
            {
                l_Write.pImageInfo = &it_Pending.Image;
            }

            l_Writes.push_back(l_Write);
        }

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(l_Writes.size()), l_Writes.data(), 0, nullptr);
        m_Pending.clear();
    }

    void VulkanDescriptorSet::EnqueueBufferWrite(uint32_t binding, uint32_t arrayElement, VkDescriptorType type, VkBuffer buffer, uint64_t offset, uint64_t range)
    {
        PendingWrite l_Write{};
        l_Write.Binding = binding;
        l_Write.ArrayElement = arrayElement;
        l_Write.Type = type;
        l_Write.IsBuffer = true;
        l_Write.Buffer.buffer = buffer;
        l_Write.Buffer.offset = offset;
        l_Write.Buffer.range = range;

        m_Pending.push_back(l_Write);
    }

    void VulkanDescriptorSet::EnqueueImageWrite(uint32_t binding, uint32_t arrayElement, VkDescriptorType type, VkImageView view, VkSampler sampler, VkImageLayout layout)
    {
        PendingWrite l_Write{};
        l_Write.Binding = binding;
        l_Write.ArrayElement = arrayElement;
        l_Write.Type = type;
        l_Write.IsBuffer = false;
        l_Write.Image.imageView = view;
        l_Write.Image.sampler = sampler;
        l_Write.Image.imageLayout = layout;

        m_Pending.push_back(l_Write);
    }
}