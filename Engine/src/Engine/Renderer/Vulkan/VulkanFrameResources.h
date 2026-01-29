#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace Engine
{
    class VulkanDescriptors;
    class VulkanResources;

    class VulkanFrameResources
    {
    public:
        void Initialize(VkDevice device, VulkanResources* resources, VulkanDescriptors* descriptors, uint32_t maxFramesInFlight);
        void Shutdown();

        void CreateUniformBuffers();
        void DestroyUniformBuffers();
        void UpdateUniformBuffer(uint32_t frameIndex, VkExtent2D extent);

        void CreateDescriptors();
        void DestroyDescriptors();

        VkBuffer GetUniformBuffer(uint32_t frameIndex) const;
        VkDeviceMemory GetUniformBufferMemory(uint32_t frameIndex) const;
        void* GetUniformBufferMapped(uint32_t frameIndex) const;
        VkDescriptorSet GetDescriptorSet(uint32_t frameIndex) const;

        uint32_t GetFrameCount() const { return m_MaxFramesInFlight; }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VulkanResources* m_Resources = nullptr;
        VulkanDescriptors* m_Descriptors = nullptr;
        uint32_t m_MaxFramesInFlight = 0;

        std::vector<VkBuffer> m_UniformBuffers;
        std::vector<VkDeviceMemory> m_UniformBuffersMemory;
        std::vector<void*> m_UniformBuffersMapped;
        std::vector<VkDescriptorSet> m_DescriptorSets;
    };
}