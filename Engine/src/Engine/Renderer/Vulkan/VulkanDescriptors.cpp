#include "Engine/Renderer/Vulkan/VulkanDescriptors.h"

#include "Engine/Utilities/Utilities.h"

#include <cstdlib>

namespace Engine
{
    void VulkanDescriptors::Initialize(VkDevice device)
    {
        m_Device = device;
    }

    void VulkanDescriptors::Shutdown()
    {
        DestroyPool();
        DestroyLayout();

        m_Device = VK_NULL_HANDLE;
    }

    void VulkanDescriptors::CreateLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
    {
        DestroyLayout();

        VkDescriptorSetLayoutCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        l_Info.bindingCount = static_cast<uint32_t>(bindings.size());
        l_Info.pBindings = bindings.empty() ? nullptr : bindings.data();

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateDescriptorSetLayout(m_Device, &l_Info, nullptr, &m_Layout), "vkCreateDescriptorSetLayout");
    }

    void VulkanDescriptors::DestroyLayout()
    {
        if (m_Layout)
        {
            vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
            m_Layout = VK_NULL_HANDLE;
        }
    }

    void VulkanDescriptors::CreatePool(uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes)
    {
        DestroyPool();

        VkDescriptorPoolCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_Info.maxSets = maxSets;
        l_Info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        l_Info.pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data();

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateDescriptorPool(m_Device, &l_Info, nullptr, &m_Pool), "vkCreateDescriptorPool");
    }

    void VulkanDescriptors::DestroyPool()
    {
        if (m_Pool)
        {
            vkDestroyDescriptorPool(m_Device, m_Pool, nullptr);
            m_Pool = VK_NULL_HANDLE;
        }
    }

    void VulkanDescriptors::AllocateSets(uint32_t setCount, std::vector<VkDescriptorSet>& outSets) const
    {
        if (!m_Pool || !m_Layout)
        {
            TR_CORE_CRITICAL("VulkanDescriptors::AllocateSets called without pool/layout.");

            std::abort();
        }

        outSets.clear();
        outSets.resize(setCount);

        std::vector<VkDescriptorSetLayout> l_Layouts(setCount, m_Layout);

        VkDescriptorSetAllocateInfo l_Alloc{};
        l_Alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_Alloc.descriptorPool = m_Pool;
        l_Alloc.descriptorSetCount = setCount;
        l_Alloc.pSetLayouts = l_Layouts.data();

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkAllocateDescriptorSets(m_Device, &l_Alloc, outSets.data()), "vkAllocateDescriptorSets");
    }

    void VulkanDescriptors::UpdateBuffer(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkBuffer buffer, VkDeviceSize range, VkDeviceSize offset) const
    {
        VkDescriptorBufferInfo l_BufferInfo{};
        l_BufferInfo.buffer = buffer;
        l_BufferInfo.offset = offset;
        l_BufferInfo.range = range;

        VkWriteDescriptorSet l_Write{};
        l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_Write.dstSet = set;
        l_Write.dstBinding = binding;
        l_Write.dstArrayElement = 0;
        l_Write.descriptorType = type;
        l_Write.descriptorCount = 1;
        l_Write.pBufferInfo = &l_BufferInfo;

        vkUpdateDescriptorSets(m_Device, 1, &l_Write, 0, nullptr);
    }

    void VulkanDescriptors::UpdateImage(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout) const
    {
        VkDescriptorImageInfo l_ImageInfo{};
        l_ImageInfo.imageLayout = imageLayout;
        l_ImageInfo.imageView = imageView;
        l_ImageInfo.sampler = sampler;

        VkWriteDescriptorSet l_Write{};
        l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_Write.dstSet = set;
        l_Write.dstBinding = binding;
        l_Write.dstArrayElement = 0;
        l_Write.descriptorType = type;
        l_Write.descriptorCount = 1;
        l_Write.pImageInfo = &l_ImageInfo;

        vkUpdateDescriptorSets(m_Device, 1, &l_Write, 0, nullptr);
    }
}