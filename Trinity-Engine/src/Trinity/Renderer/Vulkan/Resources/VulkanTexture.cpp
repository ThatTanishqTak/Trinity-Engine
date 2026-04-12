#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

namespace Trinity
{
    VulkanTexture::VulkanTexture(VkDevice device, VmaAllocator allocator, const TextureSpecification& specification) : m_Device(device), m_Allocator(allocator)
    {
        m_Specification = specification;
        m_VkFormat = VulkanUtilities::ToVkFormat(specification.Format);

        CreateImage();
        CreateImageView();
    }

    VulkanTexture::~VulkanTexture()
    {
        if (m_ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, m_ImageView, nullptr);
        }

        if (m_Image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_Allocator, m_Image, m_Allocation);
        }
    }

    void VulkanTexture::CreateImage()
    {
        VkImageCreateInfo l_ImageInfo{};
        l_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        l_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
        l_ImageInfo.format = m_VkFormat;
        l_ImageInfo.extent.width = m_Specification.Width;
        l_ImageInfo.extent.height = m_Specification.Height;
        l_ImageInfo.extent.depth = 1;
        l_ImageInfo.mipLevels = m_Specification.MipLevels;
        l_ImageInfo.arrayLayers = 1;
        l_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        l_ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        l_ImageInfo.usage = VulkanUtilities::ToVkImageUsage(m_Specification.Usage);
        l_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        l_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo l_AllocInfo{};
        l_AllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VulkanUtilities::VKCheck(vmaCreateImage(m_Allocator, &l_ImageInfo, &l_AllocInfo, &m_Image, &m_Allocation, nullptr), "Failed vmaCreateImage");
    }

    void VulkanTexture::CreateImageView()
    {
        VkImageViewCreateInfo l_ViewInfo{};
        l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_ViewInfo.image = m_Image;
        l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        l_ViewInfo.format = m_VkFormat;
        l_ViewInfo.subresourceRange.aspectMask = VulkanUtilities::GetAspectFlags(m_Specification.Format);
        l_ViewInfo.subresourceRange.baseMipLevel = 0;
        l_ViewInfo.subresourceRange.levelCount = m_Specification.MipLevels;
        l_ViewInfo.subresourceRange.baseArrayLayer = 0;
        l_ViewInfo.subresourceRange.layerCount = 1;

        VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, nullptr, &m_ImageView), "Failed vkCreateImageView");
    }
}