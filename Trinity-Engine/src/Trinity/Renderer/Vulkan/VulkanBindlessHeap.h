#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <mutex>
#include <vector>

namespace Trinity
{
    class VulkanBindlessHeap
    {
    public:
        static constexpr uint32_t MaxBindlessTextures = 16384;
        static constexpr uint32_t TextureBinding = 0;
        static constexpr uint32_t InvalidIndex = UINT32_MAX;

        VulkanBindlessHeap() = default;
        ~VulkanBindlessHeap() = default;

        void Initialize(VkDevice device);
        void Shutdown();

        uint32_t RegisterTexture(VkImageView view, VkSampler sampler, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        void UnregisterTexture(uint32_t index);

        VkDescriptorSetLayout GetLayout() const { return m_Layout; }
        VkDescriptorSet GetSet() const { return m_Set; }
        bool IsInitialized() const { return m_Set != VK_NULL_HANDLE; }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkDescriptorPool m_Pool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
        VkDescriptorSet m_Set = VK_NULL_HANDLE;

        std::vector<uint32_t> m_FreeIndices;
        std::mutex m_Mutex;
    };
}