#include "Engine/Renderer/Vulkan/VulkanDescriptors.h"

#include "Engine/Renderer/Vulkan/VulkanDebugUtils.h"
#include "Engine/Renderer/Vulkan/VulkanDevice.h"
#include "Engine/Utilities/Utilities.h"

#include <array>
#include <string>

namespace Engine
{
    namespace
    {
        bool s_PassCreationInProgress = false;
        bool s_PassRecordInProgress = false;
    }

    void VulkanDescriptors::SetPassCreationInProgress(bool value)
    {
        s_PassCreationInProgress = value;
    }

    void VulkanDescriptors::SetPassRecordInProgress(bool value)
    {
        s_PassRecordInProgress = value;
    }

    bool VulkanDescriptors::IsPassCreationInProgress()
    {
        return s_PassCreationInProgress;
    }

    bool VulkanDescriptors::IsPassRecordInProgress()
    {
        return s_PassRecordInProgress;
    }

    void VulkanDescriptors::Initialize(VulkanDevice& device, uint32_t framesInFlight)
    {
        if (IsPassCreationInProgress() || IsPassRecordInProgress())
        {
            TR_CORE_ERROR("VulkanDescriptors::Initialize called during pass creation or record");

            return;
        }
        Shutdown(device);

        m_Device = &device;
        m_FramesInFlight = framesInFlight;

        if (!device.GetDevice() || framesInFlight == 0)
        {
            return;
        }

        std::array<VkDescriptorSetLayoutBinding, 2> l_GlobalBindings{};
        l_GlobalBindings[0].binding = 0;
        l_GlobalBindings[0].descriptorCount = 1;
        l_GlobalBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_GlobalBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        l_GlobalBindings[1].binding = 1;
        l_GlobalBindings[1].descriptorCount = 1;
        l_GlobalBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        l_GlobalBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo l_GlobalLayoutCreateInfo{};
        l_GlobalLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        l_GlobalLayoutCreateInfo.bindingCount = static_cast<uint32_t>(l_GlobalBindings.size());
        l_GlobalLayoutCreateInfo.pBindings = l_GlobalBindings.data();

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateDescriptorSetLayout(device.GetDevice(), &l_GlobalLayoutCreateInfo, nullptr, &m_GlobalSetLayout), 
            "vkCreateDescriptorSetLayout(Global)");
        const VulkanDebugUtils* l_DebugUtils = device.GetDebugUtils();
        if (l_DebugUtils && m_GlobalSetLayout)
        {
            l_DebugUtils->SetObjectName(device.GetDevice(), VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, static_cast<uint64_t>(m_GlobalSetLayout), "GlobalSetLayout");
        }

        VkDescriptorSetLayoutBinding l_MaterialBinding{};
        l_MaterialBinding.binding = 0;
        l_MaterialBinding.descriptorCount = 1;
        l_MaterialBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_MaterialBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo l_MaterialLayoutCreateInfo{};
        l_MaterialLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        l_MaterialLayoutCreateInfo.bindingCount = 1;
        l_MaterialLayoutCreateInfo.pBindings = &l_MaterialBinding;

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateDescriptorSetLayout(device.GetDevice(), &l_MaterialLayoutCreateInfo, nullptr, &m_MaterialSetLayout), 
            "vkCreateDescriptorSetLayout(Material)");
        if (l_DebugUtils && m_MaterialSetLayout)
        {
            l_DebugUtils->SetObjectName(device.GetDevice(), VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, static_cast<uint64_t>(m_MaterialSetLayout), "MaterialSetLayout");
        }

        constexpr uint32_t s_MaxUniformBuffers = 512;
        constexpr uint32_t s_MaxStorageBuffers = 512;
        constexpr uint32_t s_MaxImageSamplers = 512;
        constexpr uint32_t s_MaxDescriptorSets = 512;

        std::array<VkDescriptorPoolSize, 3> l_PoolSizes{};
        l_PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_PoolSizes[0].descriptorCount = s_MaxUniformBuffers;
        l_PoolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        l_PoolSizes[1].descriptorCount = s_MaxStorageBuffers;
        l_PoolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_PoolSizes[2].descriptorCount = s_MaxImageSamplers;

        m_Pools.assign(framesInFlight, VK_NULL_HANDLE);
        for (uint32_t l_FrameIndex = 0; l_FrameIndex < framesInFlight; ++l_FrameIndex)
        {
            VkDescriptorPoolCreateInfo l_PoolCreateInfo{};
            l_PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            l_PoolCreateInfo.poolSizeCount = static_cast<uint32_t>(l_PoolSizes.size());
            l_PoolCreateInfo.pPoolSizes = l_PoolSizes.data();
            l_PoolCreateInfo.maxSets = s_MaxDescriptorSets;

            Utilities::VulkanUtilities::VKCheckStrict(vkCreateDescriptorPool(device.GetDevice(), &l_PoolCreateInfo, nullptr, &m_Pools[l_FrameIndex]), "vkCreateDescriptorPool");
            if (l_DebugUtils && m_Pools[l_FrameIndex])
            {
                const std::string l_PoolName = "DescriptorPool_Frame" + std::to_string(l_FrameIndex);
                l_DebugUtils->SetObjectName(device.GetDevice(), VK_OBJECT_TYPE_DESCRIPTOR_POOL, static_cast<uint64_t>(m_Pools[l_FrameIndex]), l_PoolName.c_str());
            }
        }
    }

    void VulkanDescriptors::Shutdown(VulkanDevice& device)
    {
        Shutdown(device, {});
    }

    void VulkanDescriptors::Shutdown(VulkanDevice& device, const std::function<void(std::function<void()>&&)>& submitResourceFree)
    {
        if (!device.GetDevice())
        {
            m_Pools.clear();
            m_GlobalSetLayout = VK_NULL_HANDLE;
            m_MaterialSetLayout = VK_NULL_HANDLE;
            m_Device = nullptr;
            m_FramesInFlight = 0;

            return;
        }

        std::vector<VkDescriptorPool> l_Pools = m_Pools;
        VkDescriptorSetLayout l_GlobalLayout = m_GlobalSetLayout;
        VkDescriptorSetLayout l_MaterialLayout = m_MaterialSetLayout;
        VulkanDevice* l_Device = &device;

        m_Pools.clear();
        m_GlobalSetLayout = VK_NULL_HANDLE;
        m_MaterialSetLayout = VK_NULL_HANDLE;
        m_Device = nullptr;
        m_FramesInFlight = 0;

        if (submitResourceFree)
        {
            submitResourceFree([l_Device, l_Pools, l_GlobalLayout, l_MaterialLayout]() mutable
            {
                if (!l_Device || !l_Device->GetDevice())
                {
                    return;
                }

                for (VkDescriptorPool l_Pool : l_Pools)
                {
                    if (l_Pool != VK_NULL_HANDLE)
                    {
                        vkDestroyDescriptorPool(l_Device->GetDevice(), l_Pool, nullptr);
                    }
                }

                if (l_GlobalLayout != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorSetLayout(l_Device->GetDevice(), l_GlobalLayout, nullptr);
                }

                if (l_MaterialLayout != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorSetLayout(l_Device->GetDevice(), l_MaterialLayout, nullptr);
                }
            });

            return;
        }

        for (VkDescriptorPool l_Pool : l_Pools)
        {
            if (l_Pool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(device.GetDevice(), l_Pool, nullptr);
            }
        }

        if (l_GlobalLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device.GetDevice(), l_GlobalLayout, nullptr);
        }

        if (l_MaterialLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device.GetDevice(), l_MaterialLayout, nullptr);
        }
    }

    void VulkanDescriptors::OnBeginFrame(VulkanDevice& device, uint32_t frameIndex)
    {
        if (!device.GetDevice())
        {
            return;
        }

        if (frameIndex >= m_Pools.size())
        {
            return;
        }

        Utilities::VulkanUtilities::VKCheckStrict(vkResetDescriptorPool(device.GetDevice(), m_Pools[frameIndex], 0), "vkResetDescriptorPool");
    }

    VkDescriptorSet VulkanDescriptors::AllocateGlobalSet(uint32_t frameIndex) const
    {
        if (!m_Device || !m_Device->GetDevice() || m_GlobalSetLayout == VK_NULL_HANDLE)
        {
            return VK_NULL_HANDLE;
        }

        if (frameIndex >= m_Pools.size())
        {
            return VK_NULL_HANDLE;
        }

        VkDescriptorSet l_Set = VK_NULL_HANDLE;
        VkDescriptorSetAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_AllocateInfo.descriptorPool = m_Pools[frameIndex];
        l_AllocateInfo.descriptorSetCount = 1;
        l_AllocateInfo.pSetLayouts = &m_GlobalSetLayout;

        Utilities::VulkanUtilities::VKCheckStrict(vkAllocateDescriptorSets(m_Device->GetDevice(), &l_AllocateInfo, &l_Set), "vkAllocateDescriptorSets(Global)");

        return l_Set;
    }

    VkDescriptorSet VulkanDescriptors::AllocateMaterialSet(uint32_t frameIndex) const
    {
        if (!m_Device || !m_Device->GetDevice() || m_MaterialSetLayout == VK_NULL_HANDLE)
        {
            return VK_NULL_HANDLE;
        }

        if (frameIndex >= m_Pools.size())
        {
            return VK_NULL_HANDLE;
        }

        VkDescriptorSet l_Set = VK_NULL_HANDLE;
        VkDescriptorSetAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_AllocateInfo.descriptorPool = m_Pools[frameIndex];
        l_AllocateInfo.descriptorSetCount = 1;
        l_AllocateInfo.pSetLayouts = &m_MaterialSetLayout;

        Utilities::VulkanUtilities::VKCheckStrict(vkAllocateDescriptorSets(m_Device->GetDevice(), &l_AllocateInfo, &l_Set), "vkAllocateDescriptorSets(Material)");

        return l_Set;
    }

    void VulkanDescriptors::WriteBuffer(VkDescriptorSet descriptorSet, uint32_t binding, const VkDescriptorBufferInfo& bufferInfo, VkDescriptorType descriptorType) const
    {
        if (!m_Device || !m_Device->GetDevice() || descriptorSet == VK_NULL_HANDLE)
        {
            return;
        }

        VkWriteDescriptorSet l_Write{};
        l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_Write.dstSet = descriptorSet;
        l_Write.dstBinding = binding;
        l_Write.descriptorCount = 1;
        l_Write.descriptorType = descriptorType;
        l_Write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &l_Write, 0, nullptr);
    }

    void VulkanDescriptors::WriteImageSampler(VkDescriptorSet descriptorSet, uint32_t binding, const VkDescriptorImageInfo& imageInfo) const
    {
        if (!m_Device || !m_Device->GetDevice() || descriptorSet == VK_NULL_HANDLE)
        {
            return;
        }

        VkWriteDescriptorSet l_Write{};
        l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_Write.dstSet = descriptorSet;
        l_Write.dstBinding = binding;
        l_Write.descriptorCount = 1;
        l_Write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_Write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &l_Write, 0, nullptr);
    }
}