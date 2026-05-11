#pragma once

#include "Trinity/Renderer/Resources/Texture.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Trinity
{
    class VulkanDevice;

    class VulkanTransientPool
    {
    public:
        struct AllocationRequest
        {
            TextureSpecification Spec;
            uint32_t FirstUse = 0;
            uint32_t LastUse = 0;
        };

        struct AllocationResult
        {
            std::shared_ptr<Texture> Texture;
            uint32_t BlockIndex = 0;
            VkDeviceSize Offset = 0;
            VkDeviceSize Size = 0;
            bool AliasedReuse = false;
        };

        VulkanTransientPool() = default;
        ~VulkanTransientPool();

        void Initialize(const VulkanDevice& device, VkDeviceSize defaultBlockSize = 64ull * 1024ull * 1024ull);
        void Shutdown();

        void Reset();
        std::vector<AllocationResult> AllocateBatch(const std::vector<AllocationRequest>& requests);

        VkDeviceSize GetTotalCapacityBytes() const;
        VkDeviceSize GetLastBatchUsedBytes() const { return m_LastUsedBytes; }
        VkDeviceSize GetLastBatchSumBytes() const { return m_LastSumBytes; }
        VkDeviceSize GetLastAliasingSavingsBytes() const { return m_LastSumBytes > m_LastUsedBytes ? m_LastSumBytes - m_LastUsedBytes : 0; }
        uint32_t GetBlockCount() const { return static_cast<uint32_t>(m_Blocks.size()); }

    private:
        struct Block
        {
            VkDeviceMemory Memory = VK_NULL_HANDLE;
            VkDeviceSize Size = 0;
            uint32_t MemoryTypeIndex = 0;
            uint32_t MemoryTypeBits = 0;
        };

        struct LiveImage
        {
            VkImage Image = VK_NULL_HANDLE;
            VkImageView View = VK_NULL_HANDLE;
        };

        uint32_t EnsureBlock(VkDeviceSize size, uint32_t memoryTypeBits);
        int32_t FindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags requiredFlags) const;
        void DestroyLiveImages();

        VkImage CreateImageForRequest(const AllocationRequest& request, VkMemoryRequirements& outRequirements);
        std::shared_ptr<Texture> WrapAsTexture(VkImage image, VkImageView view, const TextureSpecification& spec);

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkPhysicalDeviceMemoryProperties m_MemoryProperties{};
        VkDeviceSize m_BufferImageGranularity = 1;
        VkDeviceSize m_DefaultBlockSize = 0;

        std::vector<Block> m_Blocks;
        std::vector<LiveImage> m_LiveImages;

        VkDeviceSize m_LastUsedBytes = 0;
        VkDeviceSize m_LastSumBytes = 0;
    };
}