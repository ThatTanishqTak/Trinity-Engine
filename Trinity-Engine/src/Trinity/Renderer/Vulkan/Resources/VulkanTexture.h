#pragma once

#include "Trinity/Renderer/Resources/Texture.h"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanUploadQueue;

    class VulkanTexture final : public Texture
    {
    public:
        VulkanTexture(VkDevice device, VmaAllocator allocator, const TextureSpecification& specification, VulkanUploadQueue* uploadQueue = nullptr);
        VulkanTexture(VkDevice device, VkImage externalImage, VkImageView externalView, const TextureSpecification& specification);
        ~VulkanTexture() override;

        uint64_t GetOpaqueHandle() const override { return reinterpret_cast<uint64_t>(m_ImageView); }
        uint32_t GetWidth() const override { return m_Specification.Width; }
        uint32_t GetHeight() const override { return m_Specification.Height; }
        uint32_t GetDepth() const override { return m_Specification.Depth; }
        uint32_t GetMipLevels() const override { return m_Specification.MipLevels; }
        uint32_t GetArrayLayers() const override { return m_EffectiveLayerCount; }
        uint32_t GetSampleCount() const override { return m_Specification.Samples; }
        bool IsCubemap() const override { return m_Specification.IsCubemap; }
        bool IsVolume() const override { return m_Specification.IsVolume; }

        void Upload(const void* data, uint64_t size, uint32_t mipLevel = 0, uint32_t arrayLayer = 0) override;

        VkImage GetImage() const { return m_Image; }
        VkImageView GetImageView() const { return m_ImageView; }
        VkFormat GetVkFormat() const { return m_VkFormat; }
        VkImageType GetVkImageType() const { return m_ImageType; }
        VkImageViewType GetVkImageViewType() const { return m_ViewType; }

    private:
        void CreateImage();
        void CreateImageView();
        void SetDebugName();

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VulkanUploadQueue* m_UploadQueue = nullptr;
        VkImage m_Image = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        VkFormat m_VkFormat = VK_FORMAT_UNDEFINED;
        VkImageType m_ImageType = VK_IMAGE_TYPE_2D;
        VkImageViewType m_ViewType = VK_IMAGE_VIEW_TYPE_2D;
        uint32_t m_EffectiveLayerCount = 1;
    };
}