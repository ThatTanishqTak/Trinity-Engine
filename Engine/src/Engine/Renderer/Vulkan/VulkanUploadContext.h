#pragma once

#include "Engine/Renderer/Vulkan/VulkanResources.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Engine
{
    class VulkanDevice;

    class VulkanUploadContext final
    {
    public:
        VulkanUploadContext() = default;
        ~VulkanUploadContext() = default;

        VulkanUploadContext(const VulkanUploadContext&) = delete;
        VulkanUploadContext& operator=(const VulkanUploadContext&) = delete;
        VulkanUploadContext(VulkanUploadContext&&) = delete;
        VulkanUploadContext& operator=(VulkanUploadContext&&) = delete;

        void Initialize(VulkanDevice& device);
        void Shutdown(VulkanDevice& device);

        void Begin();
        void EndAndSubmitAndWait();

        void UploadBuffer(VkBuffer destination, const void* data, VkDeviceSize size);
        void UploadImage(VkImage image, uint32_t width, uint32_t height, VkFormat format, const void* data, VkDeviceSize size, VkImageLayout finalLayout,
            uint32_t mipLevels = 1, uint32_t arrayLayers = 1);

        VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }

    private:
        void RecordImageLayoutTransition(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags,
            uint32_t mipLevels, uint32_t arrayLayers) const;
        void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t arrayLayers) const;
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkImageAspectFlags aspectFlags, uint32_t arrayLayers) const;
        static VkImageAspectFlags GetAspectFlags(VkFormat format);

    private:
        VulkanDevice* m_Device = nullptr;

        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        VkFence m_UploadFence = VK_NULL_HANDLE;
    };
}