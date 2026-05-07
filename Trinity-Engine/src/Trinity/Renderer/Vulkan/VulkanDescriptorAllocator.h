#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
    class VulkanDescriptorAllocator
    {
    public:
        VulkanDescriptorAllocator() = default;
        ~VulkanDescriptorAllocator() = default;

        void Initialize(VkDevice device, uint32_t framesInFlight);
        void Shutdown();

        void BeginFrame(uint32_t frameIndex);

        VkDescriptorSet AllocatePersistent(VkDescriptorSetLayout layout);
        VkDescriptorSet AllocateTransient(VkDescriptorSetLayout layout, uint32_t frameIndex);

        void FreePersistent(VkDescriptorSet set);

    private:
        VkDescriptorPool CreatePool(VkDescriptorPoolCreateFlags flags, uint32_t maxSets);
        VkDescriptorSet TryAllocate(VkDescriptorPool pool, VkDescriptorSetLayout layout);

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        uint32_t m_FramesInFlight = 0;

        std::vector<VkDescriptorPool> m_PersistentPools;
        VkDescriptorPool m_CurrentPersistentPool = VK_NULL_HANDLE;

        std::vector<VkDescriptorPool> m_TransientPools;
    };
}