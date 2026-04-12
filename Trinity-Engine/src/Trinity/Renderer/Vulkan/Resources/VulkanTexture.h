#pragma once

#include "Trinity/Renderer/Resources/Texture.h"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanTexture final : public Texture
    {
    public:
        VulkanTexture(VkDevice device, VmaAllocator allocator, const TextureSpecification& specification);
        ~VulkanTexture() override;

        uint64_t GetOpaqueHandle() const override { return reinterpret_cast<uint64_t>(m_ImageView); }
        uint32_t GetWidth() const override { return m_Specification.Width; }
        uint32_t GetHeight() const override { return m_Specification.Height; }
        uint32_t GetMipLevels() const override { return m_Specification.MipLevels; }

        VkImage GetImage() const { return m_Image; }
        VkImageView GetImageView() const { return m_ImageView; }
        VkFormat GetVkFormat() const { return m_VkFormat; }

    private:
        void CreateImage();
        void CreateImageView();

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VkImage m_Image = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        VkFormat m_VkFormat = VK_FORMAT_UNDEFINED;
    };
}