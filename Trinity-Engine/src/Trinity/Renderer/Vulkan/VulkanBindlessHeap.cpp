#include "Trinity/Renderer/Vulkan/VulkanBindlessHeap.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

namespace Trinity
{
    void VulkanBindlessHeap::Initialize(VkDevice device)
    {
        TR_CORE_TRACE("Initializing Vulkan Bindless Heap");

        m_Device = device;

        VkDescriptorPoolSize l_PoolSize{};
        l_PoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_PoolSize.descriptorCount = MaxBindlessTextures;

        VkDescriptorPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        l_PoolInfo.maxSets = 1;
        l_PoolInfo.poolSizeCount = 1;
        l_PoolInfo.pPoolSizes = &l_PoolSize;

        VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Device, &l_PoolInfo, nullptr, &m_Pool), "Failed vkCreateDescriptorPool");

        VkDescriptorSetLayoutBinding l_Binding{};
        l_Binding.binding = TextureBinding;
        l_Binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_Binding.descriptorCount = MaxBindlessTextures;
        l_Binding.stageFlags = VK_SHADER_STAGE_ALL;

        VkDescriptorBindingFlags l_BindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo l_BindingFlagsInfo{};
        l_BindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        l_BindingFlagsInfo.bindingCount = 1;
        l_BindingFlagsInfo.pBindingFlags = &l_BindingFlags;

        VkDescriptorSetLayoutCreateInfo l_LayoutInfo{};
        l_LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        l_LayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        l_LayoutInfo.bindingCount = 1;
        l_LayoutInfo.pBindings = &l_Binding;
        l_LayoutInfo.pNext = &l_BindingFlagsInfo;

        VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device, &l_LayoutInfo, nullptr, &m_Layout), "Failed vkCreateDescriptorSetLayout");

        uint32_t l_VariableCount = MaxBindlessTextures;

        VkDescriptorSetVariableDescriptorCountAllocateInfo l_VariableCountInfo{};
        l_VariableCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
        l_VariableCountInfo.descriptorSetCount = 1;
        l_VariableCountInfo.pDescriptorCounts = &l_VariableCount;

        VkDescriptorSetAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_AllocInfo.descriptorPool = m_Pool;
        l_AllocInfo.descriptorSetCount = 1;
        l_AllocInfo.pSetLayouts = &m_Layout;
        l_AllocInfo.pNext = &l_VariableCountInfo;

        VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Device, &l_AllocInfo, &m_Set), "Failed vkAllocateDescriptorSets");

        m_FreeIndices.resize(MaxBindlessTextures);
        for (uint32_t i = 0; i < MaxBindlessTextures; ++i)
        {
            m_FreeIndices[i] = MaxBindlessTextures - 1 - i;
        }

        TR_CORE_TRACE("Vulkan Bindless Heap Initialized (Capacity = {})", MaxBindlessTextures);
    }

    void VulkanBindlessHeap::Shutdown()
    {
        if (m_Layout == VK_NULL_HANDLE && m_Pool == VK_NULL_HANDLE)
        {
            return;
        }

        TR_CORE_TRACE("Destroying Vulkan Bindless Heap");

        const size_t l_OutstandingCount = MaxBindlessTextures - m_FreeIndices.size();
        if (l_OutstandingCount > 0)
        {
            TR_CORE_WARN("Vulkan Bindless Heap shutting down with {} outstanding texture registrations", l_OutstandingCount);
        }

        if (m_Layout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
            m_Layout = VK_NULL_HANDLE;
        }

        if (m_Pool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_Device, m_Pool, nullptr);
            m_Pool = VK_NULL_HANDLE;
        }

        m_Set = VK_NULL_HANDLE;
        m_FreeIndices.clear();
        m_Device = VK_NULL_HANDLE;

        TR_CORE_TRACE("Vulkan Bindless Heap Destroyed");
    }

    uint32_t VulkanBindlessHeap::RegisterTexture(VkImageView view, VkSampler sampler, VkImageLayout layout)
    {
        if (view == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE)
        {
            TR_CORE_ERROR("View or sampler is null");
            return InvalidIndex;
        }

        std::lock_guard<std::mutex> l_Lock(m_Mutex);

        if (m_FreeIndices.empty())
        {
            TR_CORE_ERROR("Heap exhausted (Capacity = {})", MaxBindlessTextures);
            return InvalidIndex;
        }

        const uint32_t l_Index = m_FreeIndices.back();
        m_FreeIndices.pop_back();

        VkDescriptorImageInfo l_ImageInfo{};
        l_ImageInfo.sampler = sampler;
        l_ImageInfo.imageView = view;
        l_ImageInfo.imageLayout = layout;

        VkWriteDescriptorSet l_Write{};
        l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_Write.dstSet = m_Set;
        l_Write.dstBinding = TextureBinding;
        l_Write.dstArrayElement = l_Index;
        l_Write.descriptorCount = 1;
        l_Write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_Write.pImageInfo = &l_ImageInfo;

        vkUpdateDescriptorSets(m_Device, 1, &l_Write, 0, nullptr);

        return l_Index;
    }

    void VulkanBindlessHeap::UnregisterTexture(uint32_t index)
    {
        if (index >= MaxBindlessTextures)
        {
            TR_CORE_ERROR("Index {} out of range (Capacity = {})", index, MaxBindlessTextures);
            return;
        }

        std::lock_guard<std::mutex> l_Lock(m_Mutex);
        m_FreeIndices.push_back(index);
    }
}