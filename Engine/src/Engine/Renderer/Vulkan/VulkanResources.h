#pragma once

#include <vulkan/vulkan.h>

namespace Engine
{
    class VulkanResources
    {
    public:
        void Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool);
        void Shutdown();

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

        void CreateBuffer(VkDeviceSize sizeBytes, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& outBuffer, VkDeviceMemory& outMemory) const;
        void DestroyBuffer(VkBuffer& buffer, VkDeviceMemory& memory) const;

        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize sizeBytes) const;

        void CreateVertexBufferStaged(const void* vertexData, VkDeviceSize sizeBytes, VkBuffer& outBuffer, VkDeviceMemory& outMemory) const;
        void CreateIndexBufferStaged(const void* indexData, VkDeviceSize sizeBytes, VkBuffer& outBuffer, VkDeviceMemory& outMemory) const;

        // Optional for now
        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
            VkImage& outImage, VkDeviceMemory& outMemory) const;

        VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;

        void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags) const;
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;

    private:
        VkCommandBuffer BeginSingleTimeCommands() const;
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    private:
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;

        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    };
}