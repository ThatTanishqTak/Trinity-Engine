#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"

#include "Trinity/Renderer/Vulkan/VulkanUploadQueue.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

#include <algorithm>

namespace Trinity
{
    namespace
    {
        PFN_vkSetDebugUtilsObjectNameEXT s_SetObjectName = nullptr;
        bool s_DebugUtilsLoaded = false;

        void LoadDebugUtils(VkDevice device)
        {
            if (s_DebugUtilsLoaded || device == VK_NULL_HANDLE)
            {
                return;
            }

            s_SetObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
            s_DebugUtilsLoaded = true;
        }

        VkImageType DeriveImageType(const TextureSpecification& spec)
        {
            if (spec.IsVolume || spec.Depth > 1)
            {
                return VK_IMAGE_TYPE_3D;
            }

            if (spec.Height <= 1)
            {
                return VK_IMAGE_TYPE_1D;
            }

            return VK_IMAGE_TYPE_2D;
        }

        VkImageViewType DeriveViewType(const TextureSpecification& spec, uint32_t effectiveLayers)
        {
            if (spec.IsVolume || spec.Depth > 1)
            {
                return VK_IMAGE_VIEW_TYPE_3D;
            }

            if (spec.IsCubemap)
            {
                if (effectiveLayers > 6)
                {
                    return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                }

                return VK_IMAGE_VIEW_TYPE_CUBE;
            }

            if (spec.Height <= 1)
            {
                return effectiveLayers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            }

            return effectiveLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        }

        VkSampleCountFlagBits ToSampleCount(uint32_t samples)
        {
            switch (samples)
            {
                case 1:
                    return VK_SAMPLE_COUNT_1_BIT;
                case 2:
                    return VK_SAMPLE_COUNT_2_BIT;
                case 4:
                    return VK_SAMPLE_COUNT_4_BIT;
                case 8:
                    return VK_SAMPLE_COUNT_8_BIT;
                case 16:
                    return VK_SAMPLE_COUNT_16_BIT;
                case 32:
                    return VK_SAMPLE_COUNT_32_BIT;
                case 64:
                    return VK_SAMPLE_COUNT_64_BIT;
                default:
                    return VK_SAMPLE_COUNT_1_BIT;
            }
        }
    }

    VulkanTexture::VulkanTexture(VkDevice device, VmaAllocator allocator, const TextureSpecification& specification, VulkanUploadQueue* uploadQueue) : m_Device(device), m_Allocator(allocator), m_UploadQueue(uploadQueue)
    {
        m_Specification = specification;
        m_VkFormat = VulkanUtilities::ToVkFormat(specification.Format);

        m_EffectiveLayerCount = specification.IsCubemap ? (specification.ArrayLayers * 6) : specification.ArrayLayers;
        m_ImageType = DeriveImageType(specification);
        m_ViewType = DeriveViewType(specification, m_EffectiveLayerCount);

        LoadDebugUtils(m_Device);

        CreateImage();
        CreateImageView();
        SetDebugName();
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
        l_ImageInfo.imageType = m_ImageType;
        l_ImageInfo.format = m_VkFormat;
        l_ImageInfo.extent.width = m_Specification.Width;
        l_ImageInfo.extent.height = m_Specification.Height;
        l_ImageInfo.extent.depth = m_Specification.Depth;
        l_ImageInfo.mipLevels = m_Specification.MipLevels;
        l_ImageInfo.arrayLayers = m_EffectiveLayerCount;
        l_ImageInfo.samples = ToSampleCount(m_Specification.Samples);
        l_ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        l_ImageInfo.usage = VulkanUtilities::ToVkImageUsage(m_Specification.Usage);
        if (m_UploadQueue != nullptr)
        {
            l_ImageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        l_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        l_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (m_Specification.IsCubemap)
        {
            l_ImageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        if (m_Specification.IsVolume || m_Specification.Depth > 1)
        {
            l_ImageInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
        }

        VmaAllocationCreateInfo l_AllocateInfo{};
        l_AllocateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VulkanUtilities::VKCheck(vmaCreateImage(m_Allocator, &l_ImageInfo, &l_AllocateInfo, &m_Image, &m_Allocation, nullptr), "Failed vmaCreateImage");
    }

    void VulkanTexture::CreateImageView()
    {
        VkImageViewCreateInfo l_ViewInfo{};
        l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_ViewInfo.image = m_Image;
        l_ViewInfo.viewType = m_ViewType;
        l_ViewInfo.format = m_VkFormat;
        l_ViewInfo.subresourceRange.aspectMask = VulkanUtilities::GetAspectFlags(m_Specification.Format);
        l_ViewInfo.subresourceRange.baseMipLevel = 0;
        l_ViewInfo.subresourceRange.levelCount = m_Specification.MipLevels;
        l_ViewInfo.subresourceRange.baseArrayLayer = 0;
        l_ViewInfo.subresourceRange.layerCount = m_EffectiveLayerCount;

        VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, nullptr, &m_ImageView), "Failed vkCreateImageView");
    }

    void VulkanTexture::SetDebugName()
    {
        if (s_SetObjectName == nullptr || m_Specification.DebugName.empty())
        {
            return;
        }

        VkDebugUtilsObjectNameInfoEXT l_NameInfo{};
        l_NameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        l_NameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        l_NameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Image);
        l_NameInfo.pObjectName = m_Specification.DebugName.c_str();
        s_SetObjectName(m_Device, &l_NameInfo);

        VkDebugUtilsObjectNameInfoEXT l_ViewNameInfo{};
        l_ViewNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        l_ViewNameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        l_ViewNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_ImageView);
        l_ViewNameInfo.pObjectName = m_Specification.DebugName.c_str();
        s_SetObjectName(m_Device, &l_ViewNameInfo);
    }

    void VulkanTexture::Upload(const void* data, uint64_t size, uint32_t mipLevel, uint32_t arrayLayer)
    {
        if (data == nullptr || size == 0)
        {
            TR_CORE_WARN("VulkanTexture::Upload [{}] called with empty data (size={}, data={})", m_Specification.DebugName, size, data != nullptr);
            return;
        }

        if (m_UploadQueue == nullptr)
        {
            TR_CORE_ERROR("VulkanTexture::Upload [{}] called without an upload queue", m_Specification.DebugName);
            return;
        }

        if (mipLevel >= m_Specification.MipLevels)
        {
            TR_CORE_ERROR("VulkanTexture::Upload [{}] mipLevel {} out of range (mipCount={})", m_Specification.DebugName, mipLevel, m_Specification.MipLevels);
            return;
        }

        if (arrayLayer >= m_EffectiveLayerCount)
        {
            TR_CORE_ERROR("VulkanTexture::Upload [{}] arrayLayer {} out of range (layerCount={})", m_Specification.DebugName, arrayLayer, m_EffectiveLayerCount);
            return;
        }

        const uint32_t l_MipWidth = std::max<uint32_t>(1, m_Specification.Width >> mipLevel);
        const uint32_t l_MipHeight = std::max<uint32_t>(1, m_Specification.Height >> mipLevel);
        const uint32_t l_MipDepth = std::max<uint32_t>(1, m_Specification.Depth >> mipLevel);
        const VkImageAspectFlags l_Aspect = VulkanUtilities::GetAspectFlags(m_Specification.Format);

        TR_CORE_TRACE("VulkanTexture::Upload [{}] mip {} layer {} ({}x{}x{}, {} bytes)", m_Specification.DebugName, mipLevel, arrayLayer, l_MipWidth, l_MipHeight, l_MipDepth, size);

        m_UploadQueue->EnqueueTextureUpload(m_Image, l_Aspect, mipLevel, arrayLayer, l_MipWidth, l_MipHeight, l_MipDepth, data, size);
    }
}