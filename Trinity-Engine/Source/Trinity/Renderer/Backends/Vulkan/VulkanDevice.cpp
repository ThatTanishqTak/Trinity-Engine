#include <Trinity/Renderer/Backends/Vulkan/VulkanDevice.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanImGuiBackend.h>

#include <set>
#include <vector>
#include <cstring>
#include <array>

#include <Trinity/Renderer/Backends/Vulkan/VulkanSwapchain.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanUtilities.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanCommandList.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    static bool UploadViaStaging(VmaAllocator allocator, VulkanCommands& commands, VkBuffer destination, const void* data, uint64_t size, uint64_t destinationOffset)
    {
        VkBufferCreateInfo l_StagingInfo{};
        l_StagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        l_StagingInfo.size = size;
        l_StagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        l_StagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo l_StagingAllocation{};
        l_StagingAllocation.usage = VMA_MEMORY_USAGE_AUTO;
        l_StagingAllocation.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer l_StagingBuffer = VK_NULL_HANDLE;
        VmaAllocation l_StagingMemory = VK_NULL_HANDLE;
        VmaAllocationInfo l_StagingResult{};

        if (vmaCreateBuffer(allocator, &l_StagingInfo, &l_StagingAllocation, &l_StagingBuffer, &l_StagingMemory, &l_StagingResult) != VK_SUCCESS)
        {
            return false;
        }

        std::memcpy(l_StagingResult.pMappedData, data, static_cast<size_t>(size));

        commands.ImmediateSubmit([&](VkCommandBuffer commandBuffer)
        {
            VkBufferCopy l_Region{};
            l_Region.srcOffset = 0;
            l_Region.dstOffset = destinationOffset;
            l_Region.size = size;
            vkCmdCopyBuffer(commandBuffer, l_StagingBuffer, destination, 1, &l_Region);
        });

        vmaDestroyBuffer(allocator, l_StagingBuffer, l_StagingMemory);

        return true;
    }

    static bool UploadTextureViaStaging(VmaAllocator allocator, VulkanCommands& commands, VkImage image, VkImageAspectFlags aspect, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layerCount, const void* data, uint64_t size)
    {
        VkBufferCreateInfo l_StagingInfo{};
        l_StagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        l_StagingInfo.size = size;
        l_StagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        l_StagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo l_StagingAllocation{};
        l_StagingAllocation.usage = VMA_MEMORY_USAGE_AUTO;
        l_StagingAllocation.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer l_StagingBuffer = VK_NULL_HANDLE;
        VmaAllocation l_StagingMemory = VK_NULL_HANDLE;
        VmaAllocationInfo l_StagingResult{};

        if (vmaCreateBuffer(allocator, &l_StagingInfo, &l_StagingAllocation, &l_StagingBuffer, &l_StagingMemory, &l_StagingResult) != VK_SUCCESS)
        {
            return false;
        }

        std::memcpy(l_StagingResult.pMappedData, data, static_cast<size_t>(size));

        commands.ImmediateSubmit([&](VkCommandBuffer commandBuffer)
        {
            auto l_Barrier = [&](uint32_t baseLevel, uint32_t levelCount, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess)
            {
                VkImageMemoryBarrier2 l_ImageBarrier{};
                l_ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                l_ImageBarrier.srcStageMask = srcStage;
                l_ImageBarrier.srcAccessMask = srcAccess;
                l_ImageBarrier.dstStageMask = dstStage;
                l_ImageBarrier.dstAccessMask = dstAccess;
                l_ImageBarrier.oldLayout = oldLayout;
                l_ImageBarrier.newLayout = newLayout;
                l_ImageBarrier.image = image;
                l_ImageBarrier.subresourceRange.aspectMask = aspect;
                l_ImageBarrier.subresourceRange.baseMipLevel = baseLevel;
                l_ImageBarrier.subresourceRange.levelCount = levelCount;
                l_ImageBarrier.subresourceRange.baseArrayLayer = 0;
                l_ImageBarrier.subresourceRange.layerCount = layerCount;

                VkDependencyInfo l_Dependency{};
                l_Dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                l_Dependency.imageMemoryBarrierCount = 1;
                l_Dependency.pImageMemoryBarriers = &l_ImageBarrier;
                vkCmdPipelineBarrier2(commandBuffer, &l_Dependency);
            };

            l_Barrier(0, mipLevels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

            VkBufferImageCopy l_Region{};
            l_Region.bufferOffset = 0;
            l_Region.bufferRowLength = 0;
            l_Region.bufferImageHeight = 0;
            l_Region.imageSubresource.aspectMask = aspect;
            l_Region.imageSubresource.mipLevel = 0;
            l_Region.imageSubresource.baseArrayLayer = 0;
            l_Region.imageSubresource.layerCount = layerCount;
            l_Region.imageOffset = { 0, 0, 0 };
            l_Region.imageExtent = { width, height, depth };
            vkCmdCopyBufferToImage(commandBuffer, l_StagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_Region);

            if (mipLevels > 1)
            {
                int32_t l_MipWidth = static_cast<int32_t>(width);
                int32_t l_MipHeight = static_cast<int32_t>(height);

                for (uint32_t l_Level = 1; l_Level < mipLevels; ++l_Level)
                {
                    l_Barrier(l_Level - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);

                    const int32_t l_NextWidth = l_MipWidth > 1 ? l_MipWidth / 2 : 1;
                    const int32_t l_NextHeight = l_MipHeight > 1 ? l_MipHeight / 2 : 1;

                    VkImageBlit l_Blit{};
                    l_Blit.srcOffsets[0] = { 0, 0, 0 };
                    l_Blit.srcOffsets[1] = { l_MipWidth, l_MipHeight, 1 };
                    l_Blit.srcSubresource.aspectMask = aspect;
                    l_Blit.srcSubresource.mipLevel = l_Level - 1;
                    l_Blit.srcSubresource.baseArrayLayer = 0;
                    l_Blit.srcSubresource.layerCount = layerCount;
                    l_Blit.dstOffsets[0] = { 0, 0, 0 };
                    l_Blit.dstOffsets[1] = { l_NextWidth, l_NextHeight, 1 };
                    l_Blit.dstSubresource.aspectMask = aspect;
                    l_Blit.dstSubresource.mipLevel = l_Level;
                    l_Blit.dstSubresource.baseArrayLayer = 0;
                    l_Blit.dstSubresource.layerCount = layerCount;

                    vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_Blit, VK_FILTER_LINEAR);

                    l_Barrier(l_Level - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);

                    l_MipWidth = l_NextWidth;
                    l_MipHeight = l_NextHeight;
                }

                l_Barrier(mipLevels - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
            }
            else
            {
                l_Barrier(0, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
            }
        });

        vmaDestroyBuffer(allocator, l_StagingBuffer, l_StagingMemory);

        return true;
    }

    static VkDescriptorType ToVkDescriptorType(ResourceBindingType type)
    {
        switch (type)
        {
            case ResourceBindingType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case ResourceBindingType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case ResourceBindingType::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case ResourceBindingType::CombinedImageSampler:
            default: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        }
    }

    static VkImageAspectFlags DetermineAspect(Format format)
    {
        switch (format)
        {
            case Format::D32_SFLOAT: return VK_IMAGE_ASPECT_DEPTH_BIT;
            case Format::D24_UNORM_S8_UINT:
            case Format::D32_SFLOAT_S8_UINT: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            default: return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    static VkImageViewType DetermineViewType(TextureType type)
    {
        switch (type)
        {
            case TextureType::Texture2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            case TextureType::TextureCube: return VK_IMAGE_VIEW_TYPE_CUBE;
            case TextureType::Texture3D: return VK_IMAGE_VIEW_TYPE_3D;
            case TextureType::Texture2D:
            default: return VK_IMAGE_VIEW_TYPE_2D;
        }
    }

    static uint32_t ComputeMipLevels(uint32_t width, uint32_t height)
    {
        uint32_t l_Max = width > height ? width : height;
        uint32_t l_Levels = 1;
        while (l_Max > 1)
        {
            l_Max >>= 1;
            ++l_Levels;
        }

        return l_Levels;
    }

    static bool SupportsLinearBlit(VkPhysicalDevice physicalDevice, VkFormat format)
    {
        VkFormatProperties l_Properties{};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &l_Properties);

        const VkFormatFeatureFlags l_Required = VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

        return (l_Properties.optimalTilingFeatures & l_Required) == l_Required;
    }

    VulkanDevice::VulkanDevice(const NativeWindowHandle& window, const std::string& applicationName, bool enableValidation) : m_Window(window), m_ApplicationName(applicationName), m_EnableValidation(enableValidation)
    {

    }

    VulkanDevice::~VulkanDevice()
    {
        Shutdown();
    }

    bool VulkanDevice::Initialize()
    {
        if (m_Initialized)
        {
            return true;
        }

        if (!m_Instance.Initialize(m_ApplicationName, m_EnableValidation))
        {
            return false;
        }

        if (!m_Surface.Initialize(m_Instance.GetHandle(), m_Window))
        {
            return false;
        }

        VulkanFeatures l_Required;
        if (!m_PhysicalDevice.Select(m_Instance.GetHandle(), m_Surface.GetHandle(), l_Required))
        {
            return false;
        }

        if (!CreateLogicalDevice())
        {
            return false;
        }

        if (!m_Allocator.Initialize(m_Instance.GetHandle(), m_PhysicalDevice.GetHandle(), m_Device))
        {
            return false;
        }

        if (!m_Commands.Initialize(m_Device, m_GraphicsQueueFamily, m_GraphicsQueue))
        {
            return false;
        }

        if (m_EnableValidation)
        {
            m_SetObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(m_Device, "vkSetDebugUtilsObjectNameEXT"));
        }

        QueryCapabilities();

        m_ImGuiBackend = std::make_unique<VulkanImGuiBackend>(*this);

        m_Initialized = true;


        return true;
    }

    IImGuiRenderBackend& VulkanDevice::GetImGuiBackend()
    {
        return *m_ImGuiBackend;
    }

    void VulkanDevice::ReportLeaks()
    {
        uint32_t l_Pipelines = 0;
        m_Pipelines.ForEachAlive([&](VulkanPipelineResource& resource)
        {
            ++l_Pipelines;

        });

        uint32_t l_Shaders = 0;
        m_Shaders.ForEachAlive([&](VulkanShaderResource& resource)
        {
            ++l_Shaders;

        });

        uint32_t l_Samplers = 0;
        m_Samplers.ForEachAlive([&](VulkanSamplerResource& resource)
        {
            ++l_Samplers;

        });

        uint32_t l_Textures = 0;
        m_Textures.ForEachAlive([&](VulkanTextureResource& resource)
        {
            ++l_Textures;

        });

        uint32_t l_Buffers = 0;
        m_Buffers.ForEachAlive([&](VulkanBufferResource& resource)
        {
            ++l_Buffers;

        });

        const uint32_t l_Total = l_Pipelines + l_Shaders + l_Samplers + l_Textures + l_Buffers;
        if (l_Total == 0)
        {

        }
        else
        {

        }
    }

    void VulkanDevice::SetObjectName(uint64_t handle, VkObjectType type, const std::string& name)
    {
        if (m_SetObjectName == nullptr || handle == 0 || name.empty())
        {
            return;
        }

        VkDebugUtilsObjectNameInfoEXT l_NameInfo{};
        l_NameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        l_NameInfo.objectType = type;
        l_NameInfo.objectHandle = handle;
        l_NameInfo.pObjectName = name.c_str();

        m_SetObjectName(m_Device, &l_NameInfo);
    }

    void VulkanDevice::Shutdown()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device);

            for (const DeferredRelease& it_Release : m_DeferredReleases)
            {
                ReleaseNow(it_Release);
            }
            m_DeferredReleases.clear();

            ReportLeaks();

            VmaAllocator l_Allocator = m_Allocator.GetHandle();

            m_Pipelines.ForEachAlive([&](VulkanPipelineResource& resource)
            {
                if (resource.Pipeline != VK_NULL_HANDLE)
                {
                    vkDestroyPipeline(m_Device, resource.Pipeline, nullptr);
                }

                if (resource.Layout != VK_NULL_HANDLE)
                {
                    vkDestroyPipelineLayout(m_Device, resource.Layout, nullptr);
                }
            });

            m_Shaders.ForEachAlive([&](VulkanShaderResource& resource)
            {
                if (resource.Module != VK_NULL_HANDLE)
                {
                    vkDestroyShaderModule(m_Device, resource.Module, nullptr);
                }
            });

            m_Samplers.ForEachAlive([&](VulkanSamplerResource& resource)
            {
                if (resource.Sampler != VK_NULL_HANDLE)
                {
                    vkDestroySampler(m_Device, resource.Sampler, nullptr);
                }
            });

            m_Textures.ForEachAlive([&](VulkanTextureResource& resource)
            {
                if (resource.OwnsView && resource.View != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(m_Device, resource.View, nullptr);
                }

                if (resource.OwnsImage && resource.Image != VK_NULL_HANDLE)
                {
                    vmaDestroyImage(l_Allocator, resource.Image, resource.Allocation);
                }
            });

            m_Buffers.ForEachAlive([&](VulkanBufferResource& resource)
            {
                if (resource.Buffer != VK_NULL_HANDLE)
                {
                    vmaDestroyBuffer(l_Allocator, resource.Buffer, resource.Allocation);
                }
            });

            m_Commands.Shutdown();
            m_Allocator.Shutdown();

            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        m_Surface.Shutdown();
        m_Instance.Shutdown();

        m_Initialized = false;
    }

    BufferHandle VulkanDevice::CreateBuffer(const BufferDescription& description)
    {
        const bool l_DeviceLocal = description.Memory == MemoryUsage::GpuOnly;

        VkBufferUsageFlags l_BufferUsageFlags = VulkanUtilities::ToVkBufferUsage(description.Usage);
        if (l_DeviceLocal)
        {
            l_BufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        VkBufferCreateInfo l_BufferCreateInfo{};
        l_BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        l_BufferCreateInfo.size = description.Size;
        l_BufferCreateInfo.usage = l_BufferUsageFlags;
        l_BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VulkanUtilities::VmaAllocationParameters l_Parameters = VulkanUtilities::ToVmaAllocation(description.Memory);

        VmaAllocationCreateInfo l_AllocationCreateInfo{};
        l_AllocationCreateInfo.usage = l_Parameters.Usage;
        l_AllocationCreateInfo.flags = l_Parameters.Flags;

        VulkanBufferResource l_Resource{};
        l_Resource.Size = description.Size;
        l_Resource.Usage = description.Usage;
        l_Resource.Memory = description.Memory;
        l_Resource.DebugName = description.DebugName;

        VmaAllocationInfo l_Result{};
        if (vmaCreateBuffer(m_Allocator.GetHandle(), &l_BufferCreateInfo, &l_AllocationCreateInfo, &l_Resource.Buffer, &l_Resource.Allocation, &l_Result) != VK_SUCCESS)
        {

            return BufferHandle();
        }

        l_Resource.Mapped = l_Result.pMappedData;

        if (description.InitialData != nullptr)
        {
            if (l_DeviceLocal)
            {
                if (!UploadViaStaging(m_Allocator.GetHandle(), m_Commands, l_Resource.Buffer, description.InitialData, description.Size, 0))
                {

                    vmaDestroyBuffer(m_Allocator.GetHandle(), l_Resource.Buffer, l_Resource.Allocation);

                    return BufferHandle();
                }
            }
            else if (l_Resource.Mapped != nullptr)
            {
                std::memcpy(l_Resource.Mapped, description.InitialData, static_cast<size_t>(description.Size));
            }
        }

        SetObjectName(reinterpret_cast<uint64_t>(l_Resource.Buffer), VK_OBJECT_TYPE_BUFFER, l_Resource.DebugName);

        return m_Buffers.Allocate(l_Resource);
    }

    TextureHandle VulkanDevice::CreateTexture(const TextureDescription& description)
    {
        VkFormat l_Format = VulkanUtilities::ToVkFormat(description.Format);
        if (l_Format == VK_FORMAT_UNDEFINED)
        {

            return TextureHandle();
        }

        uint32_t l_ArrayLayers = description.ArrayLayers;
        VkImageCreateFlags l_ImageCreateFlags = 0;
        if (description.Type == TextureType::TextureCube)
        {
            l_ArrayLayers = description.ArrayLayers * 6;
            l_ImageCreateFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        uint32_t l_MipLevels = description.MipLevels > 0 ? description.MipLevels : 1;
        if (description.GenerateMips && description.InitialData != nullptr)
        {
            if (SupportsLinearBlit(m_PhysicalDevice.GetHandle(), l_Format))
            {
                l_MipLevels = ComputeMipLevels(description.Width, description.Height);
            }
            else
            {

                l_MipLevels = 1;
            }
        }

        VkImageCreateInfo l_ImageCreateInfo{};
        l_ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        l_ImageCreateInfo.flags = l_ImageCreateFlags;
        l_ImageCreateInfo.imageType = description.Type == TextureType::Texture3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
        l_ImageCreateInfo.format = l_Format;
        l_ImageCreateInfo.extent = { description.Width, description.Height, description.Depth };
        l_ImageCreateInfo.mipLevels = l_MipLevels;
        l_ImageCreateInfo.arrayLayers = l_ArrayLayers;
        l_ImageCreateInfo.samples = static_cast<VkSampleCountFlagBits>(description.SampleCount);
        l_ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

        VkImageUsageFlags l_Usage = VulkanUtilities::ToVkImageUsage(description.Usage);
        if (description.InitialData != nullptr)
        {
            l_Usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        if (l_MipLevels > 1)
        {
            l_Usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        l_ImageCreateInfo.usage = l_Usage;
        l_ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        l_ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo l_AllocationCreateInfo{};
        l_AllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        VulkanTextureResource l_Resource{};
        l_Resource.Format = l_Format;
        l_Resource.Extent = l_ImageCreateInfo.extent;
        l_Resource.Aspect = DetermineAspect(description.Format);
        l_Resource.OwnsImage = true;
        l_Resource.OwnsView = true;

        if (vmaCreateImage(m_Allocator.GetHandle(), &l_ImageCreateInfo, &l_AllocationCreateInfo, &l_Resource.Image, &l_Resource.Allocation, nullptr) != VK_SUCCESS)
        {

            return TextureHandle();
        }

        VkImageViewCreateInfo l_ImageViewCreateInfo{};
        l_ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_ImageViewCreateInfo.image = l_Resource.Image;
        l_ImageViewCreateInfo.viewType = DetermineViewType(description.Type);
        l_ImageViewCreateInfo.format = l_Format;
        l_ImageViewCreateInfo.subresourceRange.aspectMask = l_Resource.Aspect;
        l_ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        l_ImageViewCreateInfo.subresourceRange.levelCount = description.MipLevels;
        l_ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        l_ImageViewCreateInfo.subresourceRange.layerCount = l_ArrayLayers;

        if (vkCreateImageView(m_Device, &l_ImageViewCreateInfo, nullptr, &l_Resource.View) != VK_SUCCESS)
        {

            vmaDestroyImage(m_Allocator.GetHandle(), l_Resource.Image, l_Resource.Allocation);

            return TextureHandle();
        }

        if (description.InitialData != nullptr)
        {
            if (!UploadTextureViaStaging(m_Allocator.GetHandle(), m_Commands, l_Resource.Image, l_Resource.Aspect, description.Width, description.Height, description.Depth, description.MipLevels, l_ArrayLayers, description.InitialData, description.InitialDataSize))
            {

                vkDestroyImageView(m_Device, l_Resource.View, nullptr);
                vmaDestroyImage(m_Allocator.GetHandle(), l_Resource.Image, l_Resource.Allocation);

                return TextureHandle();
            }

            l_Resource.CurrentState = ResourceState::ShaderResource;
        }

        l_Resource.DebugName = description.DebugName;

        SetObjectName(reinterpret_cast<uint64_t>(l_Resource.Image), VK_OBJECT_TYPE_IMAGE, l_Resource.DebugName);
        SetObjectName(reinterpret_cast<uint64_t>(l_Resource.View), VK_OBJECT_TYPE_IMAGE_VIEW, l_Resource.DebugName);

        return m_Textures.Allocate(l_Resource);
    }

    SamplerHandle VulkanDevice::CreateSampler(const SamplerDescription& description)
    {
        VkSamplerCreateInfo l_SamplerCreateInfo{};
        l_SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        l_SamplerCreateInfo.magFilter = description.LinearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        l_SamplerCreateInfo.minFilter = description.LinearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        l_SamplerCreateInfo.mipmapMode = description.LinearMipmap ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        l_SamplerCreateInfo.addressModeU = description.RepeatU ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerCreateInfo.addressModeV = description.RepeatV ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerCreateInfo.addressModeW = description.RepeatW ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerCreateInfo.mipLodBias = 0.0f;
        l_SamplerCreateInfo.minLod = 0.0f;
        l_SamplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
        l_SamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        l_SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
        l_SamplerCreateInfo.compareEnable = VK_FALSE;
        l_SamplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        if (description.MaxAnisotropy > 1.0f && m_Capabilities.SupportsAnisotropy)
        {
            l_SamplerCreateInfo.anisotropyEnable = VK_TRUE;
            l_SamplerCreateInfo.maxAnisotropy = description.MaxAnisotropy < 16.0f ? description.MaxAnisotropy : 16.0f;
        }
        else
        {
            l_SamplerCreateInfo.anisotropyEnable = VK_FALSE;
            l_SamplerCreateInfo.maxAnisotropy = 1.0f;
        }

        VulkanSamplerResource l_Resource{};
        l_Resource.DebugName = description.DebugName;

        if (vkCreateSampler(m_Device, &l_SamplerCreateInfo, nullptr, &l_Resource.Sampler) != VK_SUCCESS)
        {

            return SamplerHandle();
        }

        SetObjectName(reinterpret_cast<uint64_t>(l_Resource.Sampler), VK_OBJECT_TYPE_SAMPLER, l_Resource.DebugName);

        return m_Samplers.Allocate(l_Resource);
    }

    ShaderHandle VulkanDevice::CreateShader(const ShaderDescription& description)
    {
        if (description.Bytecode.empty() || (description.Bytecode.size() % 4) != 0)
        {

            return ShaderHandle();
        }

        VkShaderModuleCreateInfo l_ModuleCreateInfo{};
        l_ModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        l_ModuleCreateInfo.codeSize = description.Bytecode.size();
        l_ModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(description.Bytecode.data());

        VulkanShaderResource l_Resource{};
        l_Resource.Stage = description.Stage;
        l_Resource.EntryPoint = description.EntryPoint;
        l_Resource.DebugName = description.DebugName;

        if (vkCreateShaderModule(m_Device, &l_ModuleCreateInfo, nullptr, &l_Resource.Module) != VK_SUCCESS)
        {

            return ShaderHandle();
        }

        SetObjectName(reinterpret_cast<uint64_t>(l_Resource.Module), VK_OBJECT_TYPE_SHADER_MODULE, l_Resource.DebugName);

        return m_Shaders.Allocate(l_Resource);
    }

    PipelineHandle VulkanDevice::CreatePipeline(const PipelineDescription& description)
    {
        VulkanShaderResource* l_Vertex = m_Shaders.Get(description.VertexShader);
        VulkanShaderResource* l_Fragment = m_Shaders.Get(description.FragmentShader);
        if (l_Vertex == nullptr || l_Fragment == nullptr)
        {

            return PipelineHandle();
        }

        std::array<VkPipelineShaderStageCreateInfo, 2> l_Stages{};
        l_Stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_Stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        l_Stages[0].module = l_Vertex->Module;
        l_Stages[0].pName = l_Vertex->EntryPoint.c_str();
        l_Stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_Stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        l_Stages[1].module = l_Fragment->Module;
        l_Stages[1].pName = l_Fragment->EntryPoint.c_str();

        VkVertexInputBindingDescription l_Binding{};
        l_Binding.binding = 0;
        l_Binding.stride = description.Vertex.Stride;
        l_Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> l_VertexInputAttributeDescriptions;
        l_VertexInputAttributeDescriptions.reserve(description.Vertex.Attributes.size());
        for (const VertexAttribute& l_Attribute : description.Vertex.Attributes)
        {
            VkVertexInputAttributeDescription l_VertexInputAttributeDescription{};
            l_VertexInputAttributeDescription.location = l_Attribute.Location;
            l_VertexInputAttributeDescription.binding = 0;
            l_VertexInputAttributeDescription.format = VulkanUtilities::ToVkFormat(l_Attribute.Format);
            l_VertexInputAttributeDescription.offset = l_Attribute.Offset;
            l_VertexInputAttributeDescriptions.push_back(l_VertexInputAttributeDescription);
        }

        const bool l_HasVertexInput = description.Vertex.Stride > 0 && !l_VertexInputAttributeDescriptions.empty();

        VkPipelineVertexInputStateCreateInfo l_PipelineVertexInputStateCreateInfo{};
        l_PipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        l_PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = l_HasVertexInput ? 1 : 0;
        l_PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = l_HasVertexInput ? &l_Binding : nullptr;
        l_PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(l_VertexInputAttributeDescriptions.size());
        l_PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = l_VertexInputAttributeDescriptions.empty() ? nullptr : l_VertexInputAttributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo l_PipelineInputAssemblyStateCreateInfo{};
        l_PipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        l_PipelineInputAssemblyStateCreateInfo.topology = VulkanUtilities::ToVkTopology(description.Topology);

        VkPipelineViewportStateCreateInfo l_PipelineViewportStateCreateInfo{};
        l_PipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        l_PipelineViewportStateCreateInfo.viewportCount = 1;
        l_PipelineViewportStateCreateInfo.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo l_PipelineRasterizationStateCreateInfo{};
        l_PipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        l_PipelineRasterizationStateCreateInfo.polygonMode = description.Rasterizer.Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        l_PipelineRasterizationStateCreateInfo.cullMode = VulkanUtilities::ToVkCullMode(description.Rasterizer.Cull);
        l_PipelineRasterizationStateCreateInfo.frontFace = VulkanUtilities::ToVkFrontFace(description.Rasterizer.Front);
        l_PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo l_PipelineMultisampleStateCreateInfo{};
        l_PipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        l_PipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo l_PipelineDepthStencilStateCreateInfo{};
        l_PipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        l_PipelineDepthStencilStateCreateInfo.depthTestEnable = description.DepthStencil.DepthTest ? VK_TRUE : VK_FALSE;
        l_PipelineDepthStencilStateCreateInfo.depthWriteEnable = description.DepthStencil.DepthWrite ? VK_TRUE : VK_FALSE;
        l_PipelineDepthStencilStateCreateInfo.depthCompareOp = VulkanUtilities::ToVkCompareOp(description.DepthStencil.DepthCompare);

        VkPipelineColorBlendAttachmentState l_PipelineColorBlendAttachmentState{};
        l_PipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        l_PipelineColorBlendAttachmentState.blendEnable = description.Blend.Enabled ? VK_TRUE : VK_FALSE;
        l_PipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        l_PipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        l_PipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        l_PipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        l_PipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        l_PipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        std::vector<VkPipelineColorBlendAttachmentState> l_BlendAttachments(description.ColorFormats.size(), l_PipelineColorBlendAttachmentState);

        VkPipelineColorBlendStateCreateInfo l_PipelineColorBlendStateCreateInfo{};
        l_PipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        l_PipelineColorBlendStateCreateInfo.attachmentCount = static_cast<uint32_t>(l_BlendAttachments.size());
        l_PipelineColorBlendStateCreateInfo.pAttachments = l_BlendAttachments.empty() ? nullptr : l_BlendAttachments.data();

        std::array<VkDynamicState, 2> l_DynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo l_PipelineDynamicStateCreateInfo{};
        l_PipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        l_PipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(l_DynamicStates.size());
        l_PipelineDynamicStateCreateInfo.pDynamicStates = l_DynamicStates.data();

        VulkanPipelineResource l_Resource{};

        uint32_t l_MaxSet = 0;
        for (const ResourceBinding& it_Binding : description.Bindings)
        {
            l_MaxSet = it_Binding.Set > l_MaxSet ? it_Binding.Set : l_MaxSet;
        }

        std::vector<VkDescriptorSetLayout> l_SetLayouts;
        if (!description.Bindings.empty())
        {
            const uint32_t l_SetCount = l_MaxSet + 1;
            for (uint32_t l_Set = 0; l_Set < l_SetCount; ++l_Set)
            {
                std::vector<VkDescriptorSetLayoutBinding> l_LayoutBindings;
                for (const ResourceBinding& it_Binding : description.Bindings)
                {
                    if (it_Binding.Set != l_Set)
                    {
                        continue;
                    }

                    VkDescriptorSetLayoutBinding l_LayoutBinding{};
                    l_LayoutBinding.binding = it_Binding.Binding;
                    l_LayoutBinding.descriptorType = ToVkDescriptorType(it_Binding.Type);
                    l_LayoutBinding.descriptorCount = 1;
                    l_LayoutBinding.stageFlags = VulkanUtilities::ToVkShaderStages(it_Binding.Stages);
                    l_LayoutBindings.push_back(l_LayoutBinding);
                }

                VkDescriptorSetLayoutCreateInfo l_LayoutInfo{};
                l_LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                l_LayoutInfo.bindingCount = static_cast<uint32_t>(l_LayoutBindings.size());
                l_LayoutInfo.pBindings = l_LayoutBindings.empty() ? nullptr : l_LayoutBindings.data();

                VkDescriptorSetLayout l_SetLayout = VK_NULL_HANDLE;
                if (vkCreateDescriptorSetLayout(m_Device, &l_LayoutInfo, nullptr, &l_SetLayout) != VK_SUCCESS)
                {

                    for (VkDescriptorSetLayout it_Layout : l_SetLayouts)
                    {
                        vkDestroyDescriptorSetLayout(m_Device, it_Layout, nullptr);
                    }

                    return PipelineHandle();
                }

                l_SetLayouts.push_back(l_SetLayout);
            }
        }

        l_Resource.SetLayouts = l_SetLayouts;

        VkPushConstantRange l_PushConstantRange{};
        l_PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        l_PushConstantRange.offset = 0;
        l_PushConstantRange.size = description.PushConstantSize;

        VkPipelineLayoutCreateInfo l_PipelineLayoutCreateInfo{};
        l_PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_PipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(l_Resource.SetLayouts.size());
        l_PipelineLayoutCreateInfo.pSetLayouts = l_Resource.SetLayouts.empty() ? nullptr : l_Resource.SetLayouts.data();
        l_PipelineLayoutCreateInfo.pushConstantRangeCount = description.PushConstantSize > 0 ? 1 : 0;
        l_PipelineLayoutCreateInfo.pPushConstantRanges = description.PushConstantSize > 0 ? &l_PushConstantRange : nullptr;

        if (vkCreatePipelineLayout(m_Device, &l_PipelineLayoutCreateInfo, nullptr, &l_Resource.Layout) != VK_SUCCESS)
        {

            for (VkDescriptorSetLayout it_Layout : l_Resource.SetLayouts)
            {
                vkDestroyDescriptorSetLayout(m_Device, it_Layout, nullptr);
            }

            return PipelineHandle();
        }

        std::vector<VkFormat> l_ColorFormats;
        l_ColorFormats.reserve(description.ColorFormats.size());
        for (Format l_ColorFormat : description.ColorFormats)
        {
            l_ColorFormats.push_back(VulkanUtilities::ToVkFormat(l_ColorFormat));
        }

        VkPipelineRenderingCreateInfo l_PipelineRenderingCreateInfo{};
        l_PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        l_PipelineRenderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(l_ColorFormats.size());
        l_PipelineRenderingCreateInfo.pColorAttachmentFormats = l_ColorFormats.empty() ? nullptr : l_ColorFormats.data();
        l_PipelineRenderingCreateInfo.depthAttachmentFormat = VulkanUtilities::ToVkFormat(description.DepthFormat);

        VkGraphicsPipelineCreateInfo l_GraphicsPipelineCreateInfo{};
        l_GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        l_GraphicsPipelineCreateInfo.pNext = &l_PipelineRenderingCreateInfo;
        l_GraphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(l_Stages.size());
        l_GraphicsPipelineCreateInfo.pStages = l_Stages.data();
        l_GraphicsPipelineCreateInfo.pVertexInputState = &l_PipelineVertexInputStateCreateInfo;
        l_GraphicsPipelineCreateInfo.pInputAssemblyState = &l_PipelineInputAssemblyStateCreateInfo;
        l_GraphicsPipelineCreateInfo.pViewportState = &l_PipelineViewportStateCreateInfo;
        l_GraphicsPipelineCreateInfo.pRasterizationState = &l_PipelineRasterizationStateCreateInfo;
        l_GraphicsPipelineCreateInfo.pMultisampleState = &l_PipelineMultisampleStateCreateInfo;
        l_GraphicsPipelineCreateInfo.pDepthStencilState = &l_PipelineDepthStencilStateCreateInfo;
        l_GraphicsPipelineCreateInfo.pColorBlendState = &l_PipelineColorBlendStateCreateInfo;
        l_GraphicsPipelineCreateInfo.pDynamicState = &l_PipelineDynamicStateCreateInfo;
        l_GraphicsPipelineCreateInfo.layout = l_Resource.Layout;

        if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &l_GraphicsPipelineCreateInfo, nullptr, &l_Resource.Pipeline) != VK_SUCCESS)
        {

            vkDestroyPipelineLayout(m_Device, l_Resource.Layout, nullptr);
            for (VkDescriptorSetLayout it_Layout : l_Resource.SetLayouts)
            {
                vkDestroyDescriptorSetLayout(m_Device, it_Layout, nullptr);
            }

            return PipelineHandle();
        }

        l_Resource.DebugName = description.DebugName;

        SetObjectName(reinterpret_cast<uint64_t>(l_Resource.Pipeline), VK_OBJECT_TYPE_PIPELINE, l_Resource.DebugName);
        SetObjectName(reinterpret_cast<uint64_t>(l_Resource.Layout), VK_OBJECT_TYPE_PIPELINE_LAYOUT, l_Resource.DebugName);

        return m_Pipelines.Allocate(l_Resource);
    }

    void VulkanDevice::DestroyBuffer(BufferHandle handle)
    {
        VulkanBufferResource l_Resource{};
        if (m_Buffers.Free(handle, l_Resource) && l_Resource.Buffer != VK_NULL_HANDLE)
        {
            DeferredRelease l_Release{};
            l_Release.Frame = m_FrameCounter;
            l_Release.Type = DeferredRelease::Kind::Buffer;
            l_Release.Buffer = l_Resource.Buffer;
            l_Release.Allocation = l_Resource.Allocation;

            m_DeferredReleases.push_back(l_Release);
        }
    }

    void VulkanDevice::DestroyTexture(TextureHandle handle)
    {
        VulkanTextureResource l_Resource{};
        if (!m_Textures.Free(handle, l_Resource))
        {
            return;
        }

        const bool l_HasNative = (l_Resource.OwnsView && l_Resource.View != VK_NULL_HANDLE) || (l_Resource.OwnsImage && l_Resource.Image != VK_NULL_HANDLE);
        if (!l_HasNative)
        {
            return;
        }

        DeferredRelease l_Release{};
        l_Release.Frame = m_FrameCounter;
        l_Release.Type = DeferredRelease::Kind::Texture;
        l_Release.Image = l_Resource.Image;
        l_Release.View = l_Resource.View;
        l_Release.Allocation = l_Resource.Allocation;
        l_Release.OwnsImage = l_Resource.OwnsImage;
        l_Release.OwnsView = l_Resource.OwnsView;

        for (const VulkanSubresourceView& it_View : l_Resource.SubViews)
        {
            if (it_View.View != VK_NULL_HANDLE)
            {
                l_Release.ExtraViews.push_back(it_View.View);
            }
        }

        m_DeferredReleases.push_back(l_Release);
    }

    VkImageView VulkanDevice::GetRenderTargetView(TextureHandle handle, uint32_t mip, uint32_t layer)
    {
        VulkanTextureResource* l_Texture = m_Textures.Get(handle);
        if (l_Texture == nullptr)
        {
            return VK_NULL_HANDLE;
        }

        for (const VulkanSubresourceView& it_View : l_Texture->SubViews)
        {
            if (it_View.Mip == mip && it_View.Layer == layer)
            {
                return it_View.View;
            }
        }

        VkImageViewCreateInfo l_ViewInfo{};
        l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_ViewInfo.image = l_Texture->Image;
        l_ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        l_ViewInfo.format = l_Texture->Format;
        l_ViewInfo.subresourceRange.aspectMask = l_Texture->Aspect;
        l_ViewInfo.subresourceRange.baseMipLevel = mip;
        l_ViewInfo.subresourceRange.levelCount = 1;
        l_ViewInfo.subresourceRange.baseArrayLayer = layer;
        l_ViewInfo.subresourceRange.layerCount = 1;

        VkImageView l_View = VK_NULL_HANDLE;
        if (vkCreateImageView(m_Device, &l_ViewInfo, nullptr, &l_View) != VK_SUCCESS)
        {


            return VK_NULL_HANDLE;
        }

        l_Texture->SubViews.push_back({ mip, layer, l_View });

        return l_View;
    }

    void VulkanDevice::DestroySampler(SamplerHandle handle)
    {
        VulkanSamplerResource l_Resource{};
        if (m_Samplers.Free(handle, l_Resource) && l_Resource.Sampler != VK_NULL_HANDLE)
        {
            DeferredRelease l_Release{};
            l_Release.Frame = m_FrameCounter;
            l_Release.Type = DeferredRelease::Kind::Sampler;
            l_Release.Sampler = l_Resource.Sampler;

            m_DeferredReleases.push_back(l_Release);
        }
    }

    void VulkanDevice::DestroyShader(ShaderHandle handle)
    {
        VulkanShaderResource l_Resource{};
        if (m_Shaders.Free(handle, l_Resource) && l_Resource.Module != VK_NULL_HANDLE)
        {
            DeferredRelease l_Release{};
            l_Release.Frame = m_FrameCounter;
            l_Release.Type = DeferredRelease::Kind::Shader;
            l_Release.Module = l_Resource.Module;

            m_DeferredReleases.push_back(l_Release);
        }
    }

    void VulkanDevice::DestroyPipeline(PipelineHandle handle)
    {
        VulkanPipelineResource l_Resource{};
        if (m_Pipelines.Free(handle, l_Resource))
        {
            DeferredRelease l_Release{};
            l_Release.Frame = m_FrameCounter;
            l_Release.Type = DeferredRelease::Kind::Pipeline;
            l_Release.Pipeline = l_Resource.Pipeline;
            l_Release.Layout = l_Resource.Layout;
            l_Release.SetLayouts = l_Resource.SetLayouts;

            m_DeferredReleases.push_back(l_Release);
        }
    }

    void VulkanDevice::ReleaseNow(const DeferredRelease& release)
    {
        switch (release.Type)
        {
            case DeferredRelease::Kind::Buffer:
                if (release.Buffer != VK_NULL_HANDLE)
                {
                    vmaDestroyBuffer(m_Allocator.GetHandle(), release.Buffer, release.Allocation);
                }
                break;

            case DeferredRelease::Kind::Texture:
                for (VkImageView it_View : release.ExtraViews)
                {
                    if (it_View != VK_NULL_HANDLE)
                    {
                        vkDestroyImageView(m_Device, it_View, nullptr);
                    }
                }

                if (release.OwnsView && release.View != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(m_Device, release.View, nullptr);
                }

                if (release.OwnsImage && release.Image != VK_NULL_HANDLE)
                {
                    vmaDestroyImage(m_Allocator.GetHandle(), release.Image, release.Allocation);
                }
                break;

            case DeferredRelease::Kind::Sampler:
                if (release.Sampler != VK_NULL_HANDLE)
                {
                    vkDestroySampler(m_Device, release.Sampler, nullptr);
                }
                break;

            case DeferredRelease::Kind::Shader:
                if (release.Module != VK_NULL_HANDLE)
                {
                    vkDestroyShaderModule(m_Device, release.Module, nullptr);
                }
                break;

            case DeferredRelease::Kind::Pipeline:
                if (release.Pipeline != VK_NULL_HANDLE)
                {
                    vkDestroyPipeline(m_Device, release.Pipeline, nullptr);
                }

                if (release.Layout != VK_NULL_HANDLE)
                {
                    vkDestroyPipelineLayout(m_Device, release.Layout, nullptr);
                }

                for (VkDescriptorSetLayout it_Layout : release.SetLayouts)
                {
                    vkDestroyDescriptorSetLayout(m_Device, it_Layout, nullptr);
                }
                break;
        }
    }

    void VulkanDevice::CollectGarbage()
    {
        ++m_FrameCounter;

        size_t l_Write = 0;
        for (size_t l_Read = 0; l_Read < m_DeferredReleases.size(); ++l_Read)
        {
            if (m_DeferredReleases[l_Read].Frame + m_DeferredFrameDelay <= m_FrameCounter)
            {
                ReleaseNow(m_DeferredReleases[l_Read]);
            }
            else
            {
                if (l_Write != l_Read)
                {
                    m_DeferredReleases[l_Write] = m_DeferredReleases[l_Read];
                }
                ++l_Write;
            }
        }

        m_DeferredReleases.resize(l_Write);
    }

    void VulkanDevice::UpdateBuffer(BufferHandle handle, const void* data, uint64_t size, uint64_t offset)
    {
        VulkanBufferResource* l_Resource = m_Buffers.Get(handle);
        if (l_Resource == nullptr || data == nullptr || size == 0)
        {
            return;
        }

        if (l_Resource->Mapped != nullptr)
        {
            std::memcpy(static_cast<uint8_t*>(l_Resource->Mapped) + offset, data, static_cast<size_t>(size));
        }
        else
        {
            UploadViaStaging(m_Allocator.GetHandle(), m_Commands, l_Resource->Buffer, data, size, offset);
        }
    }

    std::unique_ptr<Swapchain> VulkanDevice::CreateSwapchain(const SwapchainDescription& description)
    {
        auto l_Swapchain = std::make_unique<VulkanSwapchain>(*this, description);
        if (!l_Swapchain->Initialize())
        {
            return nullptr;
        }

        return l_Swapchain;
    }

    std::unique_ptr<CommandList> VulkanDevice::CreateCommandList()
    {
        return std::make_unique<VulkanCommandList>(*this);
    }

    void VulkanDevice::Submit(CommandList& commandList)
    {
        VulkanCommandList& l_CommandList = static_cast<VulkanCommandList&>(commandList);

        VkCommandBufferSubmitInfo l_CommandBufferSubmitInfo{};
        l_CommandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        l_CommandBufferSubmitInfo.commandBuffer = l_CommandList.GetHandle();

        VkSemaphoreSubmitInfo l_WaitInfo{};
        VkSemaphoreSubmitInfo l_SignalInfo{};
        VkFence l_Fence = VK_NULL_HANDLE;

        VkSubmitInfo2 l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        l_SubmitInfo.commandBufferInfoCount = 1;
        l_SubmitInfo.pCommandBufferInfos = &l_CommandBufferSubmitInfo;

        if (m_ActiveSwapchain != nullptr)
        {
            VulkanFrameSync l_Sync = m_ActiveSwapchain->GetCurrentSync();
            l_Fence = l_Sync.Fence;

            l_WaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            l_WaitInfo.semaphore = l_Sync.Wait;
            l_WaitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

            l_SignalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            l_SignalInfo.semaphore = l_Sync.Signal;
            l_SignalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

            l_SubmitInfo.waitSemaphoreInfoCount = 1;
            l_SubmitInfo.pWaitSemaphoreInfos = &l_WaitInfo;
            l_SubmitInfo.signalSemaphoreInfoCount = 1;
            l_SubmitInfo.pSignalSemaphoreInfos = &l_SignalInfo;
        }

        vkQueueSubmit2(m_GraphicsQueue, 1, &l_SubmitInfo, l_Fence);
    }

    void VulkanDevice::WaitIdle()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device);
        }
    }

    TextureHandle VulkanDevice::RegisterExternalTexture(VkImage image, VkImageView view, VkFormat format, const VkExtent3D& extent, VkImageAspectFlags aspect)
    {
        VulkanTextureResource l_Resource{};
        l_Resource.Image = image;
        l_Resource.View = view;
        l_Resource.Format = format;
        l_Resource.Extent = extent;
        l_Resource.Aspect = aspect;
        l_Resource.OwnsImage = false;
        l_Resource.OwnsView = false;

        return m_Textures.Allocate(l_Resource);
    }

    void VulkanDevice::QueryCapabilities()
    {
        const VkPhysicalDeviceProperties& l_Properties = m_PhysicalDevice.GetProperties();

        m_Capabilities.DeviceName = l_Properties.deviceName;
        m_Capabilities.MaxTexture2DSize = l_Properties.limits.maxImageDimension2D;
        m_Capabilities.MaxPushConstantSize = l_Properties.limits.maxPushConstantsSize;
        m_Capabilities.MaxColorAttachments = l_Properties.limits.maxColorAttachments;

        VkPhysicalDeviceFeatures l_Features{};
        vkGetPhysicalDeviceFeatures(m_PhysicalDevice.GetHandle(), &l_Features);
        m_Capabilities.SupportsAnisotropy = l_Features.samplerAnisotropy == VK_TRUE;
        m_Capabilities.SupportsRayTracing = false;

        VkPhysicalDeviceMemoryProperties l_Memory{};
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice.GetHandle(), &l_Memory);

        uint64_t l_LocalMemory = 0;
        for (uint32_t l_Index = 0; l_Index < l_Memory.memoryHeapCount; ++l_Index)
        {
            if ((l_Memory.memoryHeaps[l_Index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
            {
                l_LocalMemory += l_Memory.memoryHeaps[l_Index].size;
            }
        }

        m_Capabilities.DedicatedVideoMemory = l_LocalMemory;
    }

    bool VulkanDevice::CreateLogicalDevice()
    {
        const QueueFamilyIndices& l_Families = m_PhysicalDevice.GetQueueFamilies();
        m_GraphicsQueueFamily = l_Families.Graphics.value();
        m_PresentQueueFamily = l_Families.Present.value();

        std::set<uint32_t> l_UniqueFamilies = { m_GraphicsQueueFamily, m_PresentQueueFamily };

        float l_Priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> l_QueueInfos;
        l_QueueInfos.reserve(l_UniqueFamilies.size());

        for (uint32_t l_Family : l_UniqueFamilies)
        {
            VkDeviceQueueCreateInfo l_QueueCreateInfo{};
            l_QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            l_QueueCreateInfo.queueFamilyIndex = l_Family;
            l_QueueCreateInfo.queueCount = 1;
            l_QueueCreateInfo.pQueuePriorities = &l_Priority;
            l_QueueInfos.push_back(l_QueueCreateInfo);
        }

        VkPhysicalDeviceVulkan13Features l_Vulkan13Features{};
        l_Vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        l_Vulkan13Features.dynamicRendering = VK_TRUE;
        l_Vulkan13Features.synchronization2 = VK_TRUE;

        VkPhysicalDeviceVulkan11Features l_Vulkan11Features{};
        l_Vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        l_Vulkan11Features.shaderDrawParameters = VK_TRUE;
        l_Vulkan11Features.pNext = &l_Vulkan13Features;

        VkPhysicalDeviceVulkan12Features l_Vulkan12Features{};
        l_Vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        l_Vulkan12Features.timelineSemaphore = VK_TRUE;
        l_Vulkan12Features.pNext = &l_Vulkan11Features;

        VkPhysicalDeviceFeatures2 l_Features{};
        l_Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        l_Features.pNext = &l_Vulkan12Features;

        const std::vector<const char*>& l_Extensions = m_PhysicalDevice.GetRequiredExtensions();

        VkDeviceCreateInfo l_DeviceCreateInfo{};
        l_DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        l_DeviceCreateInfo.pNext = &l_Features;
        l_DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(l_QueueInfos.size());
        l_DeviceCreateInfo.pQueueCreateInfos = l_QueueInfos.data();
        l_DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(l_Extensions.size());
        l_DeviceCreateInfo.ppEnabledExtensionNames = l_Extensions.data();

        VkResult l_Result = vkCreateDevice(m_PhysicalDevice.GetHandle(), &l_DeviceCreateInfo, nullptr, &m_Device);
        if (l_Result != VK_SUCCESS)
        {

            return false;
        }

        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_PresentQueueFamily, 0, &m_PresentQueue);

        return true;
    }
}