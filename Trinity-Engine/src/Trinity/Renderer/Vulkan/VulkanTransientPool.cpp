#include "Trinity/Renderer/Vulkan/VulkanTransientPool.h"

#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Utilities/Log.h"

#include <algorithm>
#include <tuple>

namespace Trinity
{
    namespace
    {
        VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment)
        {
            if (alignment <= 1)
            {
                return value;
            }

            return (value + alignment - 1) & ~(alignment - 1);
        }
    }

    VulkanTransientPool::~VulkanTransientPool()
    {
        if (!m_Blocks.empty() || !m_LiveImages.empty())
        {
            TR_CORE_ERROR("VulkanTransientPool destroyed without Shutdown() being called; leaking GPU memory to avoid destroying against a dead VkDevice");
        }
    }

    void VulkanTransientPool::Initialize(const VulkanDevice& device, VkDeviceSize defaultBlockSize)
    {
        TR_CORE_TRACE("Initializing Vulkan Transient Pool");

        m_Device = device.GetDevice();
        m_DefaultBlockSize = defaultBlockSize;

        const VkPhysicalDeviceProperties& l_Properties = device.GetProperties();
        m_BufferImageGranularity = l_Properties.limits.bufferImageGranularity;

        vkGetPhysicalDeviceMemoryProperties(device.GetPhysicalDevice(), &m_MemoryProperties);

        TR_CORE_TRACE("Vulkan Transient Pool Initialized (DefaultBlockSize = {} MB, BufferImageGranularity = {})", m_DefaultBlockSize / (1024ull * 1024ull), m_BufferImageGranularity);
    }

    void VulkanTransientPool::Shutdown()
    {
        if (m_Device == VK_NULL_HANDLE && m_Blocks.empty())
        {
            return;
        }

        TR_CORE_TRACE("Shutting Down Vulkan Transient Pool");

        DestroyLiveImages();

        for (auto& it_Block : m_Blocks)
        {
            if (it_Block.Memory != VK_NULL_HANDLE)
            {
                vkFreeMemory(m_Device, it_Block.Memory, nullptr);
                it_Block.Memory = VK_NULL_HANDLE;
            }
        }

        m_Blocks.clear();
        m_Device = VK_NULL_HANDLE;
        m_LastUsedBytes = 0;
        m_LastSumBytes = 0;

        TR_CORE_TRACE("Vulkan Transient Pool Shutdown Complete");
    }

    void VulkanTransientPool::Reset()
    {
        DestroyLiveImages();
        m_LastUsedBytes = 0;
        m_LastSumBytes = 0;
    }

    void VulkanTransientPool::DestroyLiveImages()
    {
        for (auto& it_Live : m_LiveImages)
        {
            if (it_Live.View != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_Device, it_Live.View, nullptr);
            }

            if (it_Live.Image != VK_NULL_HANDLE)
            {
                vkDestroyImage(m_Device, it_Live.Image, nullptr);
            }
        }
        m_LiveImages.clear();
    }

    int32_t VulkanTransientPool::FindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags requiredFlags) const
    {
        for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; ++i)
        {
            const bool l_TypeMatch = (typeFilter & (1u << i)) != 0;
            const bool l_FlagsMatch = (m_MemoryProperties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags;
            if (l_TypeMatch && l_FlagsMatch)
            {
                return static_cast<int32_t>(i);
            }
        }

        return -1;
    }

    uint32_t VulkanTransientPool::EnsureBlock(VkDeviceSize size, uint32_t memoryTypeBits)
    {
        for (uint32_t i = 0; i < m_Blocks.size(); ++i)
        {
            if ((m_Blocks[i].MemoryTypeBits & memoryTypeBits) != 0 && m_Blocks[i].Size >= size)
            {
                return i;
            }
        }

        const int32_t l_TypeIndex = FindMemoryTypeIndex(memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (l_TypeIndex < 0)
        {
            TR_CORE_CRITICAL("VulkanTransientPool: no DEVICE_LOCAL memory type matches memoryTypeBits 0x{:x}", memoryTypeBits);
            return UINT32_MAX;
        }

        const VkDeviceSize l_BlockSize = std::max(size, m_DefaultBlockSize);

        VkMemoryAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        l_AllocateInfo.allocationSize = l_BlockSize;
        l_AllocateInfo.memoryTypeIndex = static_cast<uint32_t>(l_TypeIndex);

        VkDeviceMemory l_Memory = VK_NULL_HANDLE;
        VulkanUtilities::VKCheck(vkAllocateMemory(m_Device, &l_AllocateInfo, nullptr, &l_Memory), "VulkanTransientPool: vkAllocateMemory failed");

        Block l_Block{};
        l_Block.Memory = l_Memory;
        l_Block.Size = l_BlockSize;
        l_Block.MemoryTypeIndex = static_cast<uint32_t>(l_TypeIndex);
        l_Block.MemoryTypeBits = 1u << l_TypeIndex;
        m_Blocks.push_back(l_Block);

        TR_CORE_DEBUG("VulkanTransientPool: allocated block #{} ({} MB, memoryTypeIndex={})", m_Blocks.size() - 1, l_BlockSize / (1024ull * 1024ull), l_TypeIndex);

        return static_cast<uint32_t>(m_Blocks.size() - 1);
    }

    VkImage VulkanTransientPool::CreateImageForRequest(const AllocationRequest& request, VkMemoryRequirements& outRequirements)
    {
        VkImageCreateInfo l_ImageInfo{};
        l_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        l_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
        l_ImageInfo.format = VulkanUtilities::ToVkFormat(request.Spec.Format);
        l_ImageInfo.extent.width = request.Spec.Width;
        l_ImageInfo.extent.height = request.Spec.Height;
        l_ImageInfo.extent.depth = 1;
        l_ImageInfo.mipLevels = request.Spec.MipLevels > 0 ? request.Spec.MipLevels : 1;
        l_ImageInfo.arrayLayers = request.Spec.ArrayLayers > 0 ? request.Spec.ArrayLayers : 1;
        l_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        l_ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        l_ImageInfo.usage = VulkanUtilities::ToVkImageUsage(request.Spec.Usage);
        l_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        l_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImage l_Image = VK_NULL_HANDLE;
        VulkanUtilities::VKCheck(vkCreateImage(m_Device, &l_ImageInfo, nullptr, &l_Image), "VulkanTransientPool: vkCreateImage failed");

        vkGetImageMemoryRequirements(m_Device, l_Image, &outRequirements);

        return l_Image;
    }

    std::shared_ptr<Texture> VulkanTransientPool::WrapAsTexture(VkImage image, VkImageView view, const TextureSpecification& spec)
    {
        return std::make_shared<VulkanTexture>(m_Device, image, view, spec);
    }

    std::vector<VulkanTransientPool::AllocationResult> VulkanTransientPool::AllocateBatch(const std::vector<AllocationRequest>& requests)
    {
        Reset();

        const size_t l_Count = requests.size();
        std::vector<AllocationResult> l_Results(l_Count);

        if (l_Count == 0)
        {
            return l_Results;
        }

        struct Pending
        {
            uint32_t OriginalIndex = 0;
            VkImage Image = VK_NULL_HANDLE;
            VkMemoryRequirements Requirements{};
            uint32_t FirstUse = 0;
            uint32_t LastUse = 0;
        };

        std::vector<Pending> l_Pending(l_Count);
        for (size_t i = 0; i < l_Count; ++i)
        {
            l_Pending[i].OriginalIndex = static_cast<uint32_t>(i);
            l_Pending[i].Image = CreateImageForRequest(requests[i], l_Pending[i].Requirements);
            l_Pending[i].FirstUse = requests[i].FirstUse;
            l_Pending[i].LastUse = requests[i].LastUse;

            m_LastSumBytes += l_Pending[i].Requirements.size;
        }

        std::sort(l_Pending.begin(), l_Pending.end(), [](const Pending& a, const Pending& b)
        {
            if (a.Requirements.size != b.Requirements.size)
            {
                return a.Requirements.size > b.Requirements.size;
            }

            return a.FirstUse < b.FirstUse;
        });

        struct Placement
        {
            uint32_t BlockIndex = 0;
            VkDeviceSize Offset = 0;
            VkDeviceSize Size = 0;
            uint32_t FirstUse = 0;
            uint32_t LastUse = 0;
        };

        std::vector<Placement> l_Placed;
        l_Placed.reserve(l_Count);

        VkDeviceSize l_PeakOffset = 0;

        for (auto& it_Item : l_Pending)
        {
            const VkDeviceSize l_Alignment = std::max<VkDeviceSize>(it_Item.Requirements.alignment, m_BufferImageGranularity);
            const VkDeviceSize l_Size = it_Item.Requirements.size;
            const uint32_t l_TypeBits = it_Item.Requirements.memoryTypeBits;

            bool l_Aliased = false;
            uint32_t l_TargetBlock = UINT32_MAX;
            VkDeviceSize l_TargetOffset = 0;

            for (uint32_t l_BlockIndex = 0; l_BlockIndex < m_Blocks.size(); ++l_BlockIndex)
            {
                const Block& l_Block = m_Blocks[l_BlockIndex];
                if ((l_Block.MemoryTypeBits & l_TypeBits) == 0)
                {
                    continue;
                }

                std::vector<std::tuple<VkDeviceSize, VkDeviceSize>> l_Conflicts;
                for (const auto& it_P : l_Placed)
                {
                    if (it_P.BlockIndex != l_BlockIndex)
                    {
                        continue;
                    }

                    const bool l_Overlap = !(it_P.LastUse < it_Item.FirstUse || it_P.FirstUse > it_Item.LastUse);
                    if (l_Overlap)
                    {
                        l_Conflicts.emplace_back(it_P.Offset, it_P.Size);
                    }
                }

                std::sort(l_Conflicts.begin(), l_Conflicts.end());

                VkDeviceSize l_Candidate = 0;
                bool l_Fits = false;
                for (const auto& it_C : l_Conflicts)
                {
                    const VkDeviceSize l_CStart = std::get<0>(it_C);
                    const VkDeviceSize l_CEnd = l_CStart + std::get<1>(it_C);

                    const VkDeviceSize l_Aligned = AlignUp(l_Candidate, l_Alignment);
                    if (l_Aligned + l_Size <= l_CStart)
                    {
                        l_Candidate = l_Aligned;
                        l_Fits = true;

                        break;
                    }
                    l_Candidate = l_CEnd;
                }

                if (!l_Fits)
                {
                    const VkDeviceSize l_Aligned = AlignUp(l_Candidate, l_Alignment);
                    if (l_Aligned + l_Size <= l_Block.Size)
                    {
                        l_Candidate = l_Aligned;
                        l_Fits = true;
                    }
                }

                if (l_Fits)
                {
                    l_TargetBlock = l_BlockIndex;
                    l_TargetOffset = l_Candidate;
                    l_Aliased = !l_Conflicts.empty() || std::any_of(l_Placed.begin(), l_Placed.end(), [&](const Placement& p)
                    {
                        return p.BlockIndex == l_BlockIndex;
                    });

                    break;
                }
            }

            if (l_TargetBlock == UINT32_MAX)
            {
                l_TargetBlock = EnsureBlock(l_Size, l_TypeBits);
                if (l_TargetBlock == UINT32_MAX)
                {
                    TR_CORE_ERROR("Failed to allocate block for resource (Size = {}, MemoryTypeBits = 0x{:x})", l_Size, l_TypeBits);
                    vkDestroyImage(m_Device, it_Item.Image, nullptr);

                    continue;
                }

                l_TargetOffset = 0;
                l_Aliased = false;
            }

            VulkanUtilities::VKCheck(vkBindImageMemory(m_Device, it_Item.Image, m_Blocks[l_TargetBlock].Memory, l_TargetOffset), "vkBindImageMemory failed");

            const TextureSpecification& l_Spec = requests[it_Item.OriginalIndex].Spec;

            VkImageViewCreateInfo l_ViewInfo{};
            l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            l_ViewInfo.image = it_Item.Image;
            l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            l_ViewInfo.format = VulkanUtilities::ToVkFormat(l_Spec.Format);
            l_ViewInfo.subresourceRange.aspectMask = VulkanUtilities::GetAspectFlags(l_Spec.Format);
            l_ViewInfo.subresourceRange.baseMipLevel = 0;
            l_ViewInfo.subresourceRange.levelCount = l_Spec.MipLevels > 0 ? l_Spec.MipLevels : 1;
            l_ViewInfo.subresourceRange.baseArrayLayer = 0;
            l_ViewInfo.subresourceRange.layerCount = l_Spec.ArrayLayers > 0 ? l_Spec.ArrayLayers : 1;

            VkImageView l_View = VK_NULL_HANDLE;
            VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &l_ViewInfo, nullptr, &l_View), "vkCreateImageView failed");

            LiveImage l_Live{};
            l_Live.Image = it_Item.Image;
            l_Live.View = l_View;
            m_LiveImages.push_back(l_Live);

            Placement l_P{};
            l_P.BlockIndex = l_TargetBlock;
            l_P.Offset = l_TargetOffset;
            l_P.Size = l_Size;
            l_P.FirstUse = it_Item.FirstUse;
            l_P.LastUse = it_Item.LastUse;
            l_Placed.push_back(l_P);

            l_PeakOffset = std::max(l_PeakOffset, l_TargetOffset + l_Size);

            AllocationResult& l_Result = l_Results[it_Item.OriginalIndex];
            l_Result.Texture = WrapAsTexture(it_Item.Image, l_View, l_Spec);
            l_Result.BlockIndex = l_TargetBlock;
            l_Result.Offset = l_TargetOffset;
            l_Result.Size = l_Size;
            l_Result.AliasedReuse = l_Aliased;
        }

        VkDeviceSize l_Used = 0;
        for (const auto& it_P : l_Placed)
        {
            l_Used = std::max(l_Used, it_P.Offset + it_P.Size);
        }
        m_LastUsedBytes = l_Used;

        TR_CORE_DEBUG("Batch placed {} resources (Sum = {} KB, Peak = {} KB, Savings = {} KB, Blocks = {})", l_Count, m_LastSumBytes / 1024, m_LastUsedBytes / 1024, GetLastAliasingSavingsBytes() / 1024, m_Blocks.size());

        return l_Results;
    }

    VkDeviceSize VulkanTransientPool::GetTotalCapacityBytes() const
    {
        VkDeviceSize l_Total = 0;
        for (const auto& it_Block : m_Blocks)
        {
            l_Total += it_Block.Size;
        }

        return l_Total;
    }
}