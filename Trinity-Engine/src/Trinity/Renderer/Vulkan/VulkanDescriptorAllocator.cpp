#include "Trinity/Renderer/Vulkan/VulkanDescriptorAllocator.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

namespace Trinity
{
    namespace
    {
        constexpr uint32_t l_MaxSetsPerPool = 1024;

        const VkDescriptorPoolSize k_DefaultPoolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 512 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2048 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 512 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256 },
            { VK_DESCRIPTOR_TYPE_SAMPLER, 256 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 256 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 256 }
        };

        constexpr uint32_t k_DefaultPoolSizeCount = sizeof(k_DefaultPoolSizes) / sizeof(k_DefaultPoolSizes[0]);
    }

    void VulkanDescriptorAllocator::Initialize(VkDevice device, uint32_t framesInFlight)
    {
        TR_CORE_TRACE("Initializing Descriptor Allocator");

        m_Device = device;
        m_FramesInFlight = framesInFlight;

        m_CurrentPersistentPool = CreatePool(0, l_MaxSetsPerPool);
        m_PersistentPools.push_back(m_CurrentPersistentPool);

        m_TransientPools.reserve(framesInFlight);
        for (uint32_t i = 0; i < framesInFlight; i++)
        {
            m_TransientPools.push_back(CreatePool(0, l_MaxSetsPerPool));
        }

        TR_CORE_TRACE("Descriptor Allocator Initialized");
    }

    void VulkanDescriptorAllocator::Shutdown()
    {
        TR_CORE_TRACE("Shutting Down Descriptor Allocator");

        for (auto it_Pool : m_PersistentPools)
        {
            if (it_Pool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(m_Device, it_Pool, nullptr);
            }
        }
        m_PersistentPools.clear();
        m_CurrentPersistentPool = VK_NULL_HANDLE;

        for (auto it_Pool : m_TransientPools)
        {
            if (it_Pool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(m_Device, it_Pool, nullptr);
            }
        }
        m_TransientPools.clear();

        m_Device = VK_NULL_HANDLE;
        m_FramesInFlight = 0;

        TR_CORE_TRACE("Descriptor Allocator Shutdown Complete");
    }

    void VulkanDescriptorAllocator::BeginFrame(uint32_t frameIndex)
    {
        if (frameIndex >= m_TransientPools.size())
        {
            return;
        }

        vkResetDescriptorPool(m_Device, m_TransientPools[frameIndex], 0);
    }

    VkDescriptorSet VulkanDescriptorAllocator::AllocatePersistent(VkDescriptorSetLayout layout)
    {
        VkDescriptorSet l_Set = TryAllocate(m_CurrentPersistentPool, layout);
        if (l_Set != VK_NULL_HANDLE)
        {
            return l_Set;
        }

        m_CurrentPersistentPool = CreatePool(0, l_MaxSetsPerPool);
        m_PersistentPools.push_back(m_CurrentPersistentPool);

        l_Set = TryAllocate(m_CurrentPersistentPool, layout);
        if (l_Set == VK_NULL_HANDLE)
        {
            TR_CORE_ERROR("Failed to allocate persistent descriptor set even after growing the pool");
        }

        return l_Set;
    }

    VkDescriptorSet VulkanDescriptorAllocator::AllocateTransient(VkDescriptorSetLayout layout, uint32_t frameIndex)
    {
        if (frameIndex >= m_TransientPools.size())
        {
            TR_CORE_ERROR("AllocateTransient: frameIndex {} out of range (max {})", frameIndex, m_TransientPools.size());
            return VK_NULL_HANDLE;
        }

        VkDescriptorSet l_Set = TryAllocate(m_TransientPools[frameIndex], layout);
        if (l_Set == VK_NULL_HANDLE)
        {
            TR_CORE_ERROR("AllocateTransient: pool exhausted for frame {}", frameIndex);
        }

        return l_Set;
    }

    void VulkanDescriptorAllocator::FreePersistent(VkDescriptorSet set)
    {
        if (set == VK_NULL_HANDLE)
        {
            return;
        }

        for (auto it_Pool : m_PersistentPools)
        {
            VkResult l_Result = vkFreeDescriptorSets(m_Device, it_Pool, 1, &set);
            if (l_Result == VK_SUCCESS)
            {
                return;
            }
        }
    }

    VkDescriptorPool VulkanDescriptorAllocator::CreatePool(VkDescriptorPoolCreateFlags flags, uint32_t maxSets)
    {
        VkDescriptorPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_PoolInfo.flags = flags | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        l_PoolInfo.maxSets = maxSets;
        l_PoolInfo.poolSizeCount = k_DefaultPoolSizeCount;
        l_PoolInfo.pPoolSizes = k_DefaultPoolSizes;

        VkDescriptorPool l_Pool = VK_NULL_HANDLE;
        VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Device, &l_PoolInfo, nullptr, &l_Pool), "Failed vkCreateDescriptorPool");

        return l_Pool;
    }

    VkDescriptorSet VulkanDescriptorAllocator::TryAllocate(VkDescriptorPool pool, VkDescriptorSetLayout layout)
    {
        VkDescriptorSetAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_AllocateInfo.descriptorPool = pool;
        l_AllocateInfo.descriptorSetCount = 1;
        l_AllocateInfo.pSetLayouts = &layout;

        VkDescriptorSet l_Set = VK_NULL_HANDLE;
        VkResult l_Result = vkAllocateDescriptorSets(m_Device, &l_AllocateInfo, &l_Set);

        if (l_Result == VK_ERROR_OUT_OF_POOL_MEMORY || l_Result == VK_ERROR_FRAGMENTED_POOL)
        {
            return VK_NULL_HANDLE;
        }

        if (l_Result != VK_SUCCESS)
        {
            TR_CORE_ERROR("vkAllocateDescriptorSets failed with VkResult={}", static_cast<int>(l_Result));
            return VK_NULL_HANDLE;
        }

        return l_Set;
    }
}