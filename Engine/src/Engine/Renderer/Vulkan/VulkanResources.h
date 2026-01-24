#pragma once

#include "Engine/Renderer/Vulkan/VulkanCommand.h"
#include "Engine/Renderer/Vulkan/VulkanDevice.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Engine
{
    class VulkanResources final
    {
    public:
        struct BufferResource
        {
            VkBuffer Buffer = VK_NULL_HANDLE;
            VkDeviceMemory Memory = VK_NULL_HANDLE;
            VkDeviceSize Size = 0;
        };

        struct ImageResource
        {
            VkImage Image = VK_NULL_HANDLE;
            VkDeviceMemory Memory = VK_NULL_HANDLE;
        };

        static BufferResource CreateBuffer(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        static void DestroyBuffer(VulkanDevice& device, BufferResource& buffer);

        static ImageResource CreateImage(VulkanDevice& device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
            VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, uint32_t mipLevels = 1, uint32_t arrayLayers = 1, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
        static void DestroyImage(VulkanDevice& device, ImageResource& image);

        static VkImageView CreateImageView(VulkanDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, 
            uint32_t mipLevels = 1, uint32_t baseMipLevel = 0, uint32_t baseArrayLayer = 0, uint32_t layerCount = 1);
        static void DestroyImageView(VulkanDevice& device, VkImageView& imageView);

        static VkSampler CreateSampler(VulkanDevice& device, const VkSamplerCreateInfo& createInfo);
        static void DestroySampler(VulkanDevice& device, VkSampler& sampler);

        static BufferResource CreateStagingBuffer(VulkanDevice& device, VkDeviceSize size);
        static void UploadToBuffer(VulkanDevice& device, VulkanCommand& command, VkBuffer destination, const void* data, VkDeviceSize size);

        static void UploadToImage(VulkanDevice& device, VulkanCommand& command, VkImage image, uint32_t width, uint32_t height, VkFormat format, const void* data, VkDeviceSize size, 
            VkImageLayout finalLayout, uint32_t mipLevels = 1, uint32_t arrayLayers = 1);

    private:
        static uint32_t FindMemoryType(VulkanDevice& device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
        static VkImageAspectFlags GetAspectFlags(VkFormat format);
        static void TransitionImageLayout(VulkanDevice& device, VulkanCommand& command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags, 
            uint32_t mipLevels, uint32_t arrayLayers);
        static void CopyBuffer(VulkanDevice& device, VulkanCommand& command, VkBuffer source, VkBuffer destination, VkDeviceSize size);
        static void CopyBufferToImage(VulkanDevice& device, VulkanCommand& command, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkImageAspectFlags aspectFlags, uint32_t arrayLayers);
    };
}