#include "Engine/Renderer/Vulkan/VulkanResources.h"

#include "Engine/Utilities/Utilities.h"

#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace Engine
{
    VulkanResources::BufferResource VulkanResources::CreateBuffer(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        // Create the buffer handle so we can query memory requirements.
        VkBufferCreateInfo l_BufferInfo{};
        l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        l_BufferInfo.size = size;
        l_BufferInfo.usage = usage;
        l_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        BufferResource l_Buffer{};
        Utilities::VulkanUtilities::VKCheckStrict(vkCreateBuffer(device.GetDevice(), &l_BufferInfo, nullptr, &l_Buffer.Buffer), "vkCreateBuffer");

        // Query memory requirements so we can select a compatible heap.
        VkMemoryRequirements l_MemoryRequirements{};
        vkGetBufferMemoryRequirements(device.GetDevice(), l_Buffer.Buffer, &l_MemoryRequirements);

        VkMemoryAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        l_AllocateInfo.allocationSize = l_MemoryRequirements.size;
        l_AllocateInfo.memoryTypeIndex = FindMemoryType(device, l_MemoryRequirements.memoryTypeBits, properties);

        Utilities::VulkanUtilities::VKCheckStrict(vkAllocateMemory(device.GetDevice(), &l_AllocateInfo, nullptr, &l_Buffer.Memory), "vkAllocateMemory (buffer)");

        // Bind memory after allocation so the buffer has valid storage.
        Utilities::VulkanUtilities::VKCheckStrict(vkBindBufferMemory(device.GetDevice(), l_Buffer.Buffer, l_Buffer.Memory, 0), "vkBindBufferMemory");

        l_Buffer.Size = size;

        return l_Buffer;
    }

    void VulkanResources::DestroyBuffer(VulkanDevice& device, BufferResource& buffer)
    {
        // Explicit destruction keeps ownership clear and avoids implicit device ownership.
        if (buffer.Buffer)
        {
            vkDestroyBuffer(device.GetDevice(), buffer.Buffer, nullptr);
            buffer.Buffer = VK_NULL_HANDLE;
        }

        if (buffer.Memory)
        {
            vkFreeMemory(device.GetDevice(), buffer.Memory, nullptr);
            buffer.Memory = VK_NULL_HANDLE;
        }

        buffer.Size = 0;
    }

    VulkanResources::ImageResource VulkanResources::CreateImage(VulkanDevice& device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, 
        VkMemoryPropertyFlags properties, VkImageLayout initialLayout, uint32_t mipLevels, uint32_t arrayLayers, VkSampleCountFlagBits samples)
    {
        // Create the VkImage before allocating memory so requirements are available.
        VkImageCreateInfo l_ImageInfo{};
        l_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        l_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
        l_ImageInfo.extent.width = width;
        l_ImageInfo.extent.height = height;
        l_ImageInfo.extent.depth = 1;
        l_ImageInfo.mipLevels = mipLevels;
        l_ImageInfo.arrayLayers = arrayLayers;
        l_ImageInfo.format = format;
        l_ImageInfo.tiling = tiling;
        l_ImageInfo.initialLayout = initialLayout;
        l_ImageInfo.usage = usage;
        l_ImageInfo.samples = samples;
        l_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        ImageResource l_Image{};
        Utilities::VulkanUtilities::VKCheckStrict(vkCreateImage(device.GetDevice(), &l_ImageInfo, nullptr, &l_Image.Image), "vkCreateImage");

        VkMemoryRequirements l_MemoryRequirements{};
        vkGetImageMemoryRequirements(device.GetDevice(), l_Image.Image, &l_MemoryRequirements);

        VkMemoryAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        l_AllocateInfo.allocationSize = l_MemoryRequirements.size;
        l_AllocateInfo.memoryTypeIndex = FindMemoryType(device, l_MemoryRequirements.memoryTypeBits, properties);

        Utilities::VulkanUtilities::VKCheckStrict(vkAllocateMemory(device.GetDevice(), &l_AllocateInfo, nullptr, &l_Image.Memory), "vkAllocateMemory (image)");

        // Bind the allocated memory so the image has backing storage.
        Utilities::VulkanUtilities::VKCheckStrict(vkBindImageMemory(device.GetDevice(), l_Image.Image, l_Image.Memory, 0), "vkBindImageMemory");

        return l_Image;
    }

    void VulkanResources::DestroyImage(VulkanDevice& device, ImageResource& image)
    {
        if (image.Image)
        {
            vkDestroyImage(device.GetDevice(), image.Image, nullptr);
            image.Image = VK_NULL_HANDLE;
        }

        if (image.Memory)
        {
            vkFreeMemory(device.GetDevice(), image.Memory, nullptr);
            image.Memory = VK_NULL_HANDLE;
        }
    }

    VkImageView VulkanResources::CreateImageView(VulkanDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType, uint32_t mipLevels, 
        uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t layerCount)
    {
        // Image views describe how the image will be interpreted by the pipeline.
        VkImageViewCreateInfo l_ViewInfo{};
        l_ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_ViewInfo.image = image;
        l_ViewInfo.viewType = viewType;
        l_ViewInfo.format = format;
        l_ViewInfo.subresourceRange.aspectMask = aspectFlags;
        l_ViewInfo.subresourceRange.baseMipLevel = baseMipLevel;
        l_ViewInfo.subresourceRange.levelCount = mipLevels;
        l_ViewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
        l_ViewInfo.subresourceRange.layerCount = layerCount;

        VkImageView l_ImageView = VK_NULL_HANDLE;
        Utilities::VulkanUtilities::VKCheckStrict(vkCreateImageView(device.GetDevice(), &l_ViewInfo, nullptr, &l_ImageView), "vkCreateImageView");

        return l_ImageView;
    }

    void VulkanResources::DestroyImageView(VulkanDevice& device, VkImageView& imageView)
    {
        if (imageView)
        {
            vkDestroyImageView(device.GetDevice(), imageView, nullptr);
            imageView = VK_NULL_HANDLE;
        }
    }

    VkSampler VulkanResources::CreateSampler(VulkanDevice& device, const VkSamplerCreateInfo& createInfo)
    {
        // Samplers are lightweight but still need explicit creation and destruction.
        VkSampler l_Sampler = VK_NULL_HANDLE;
        Utilities::VulkanUtilities::VKCheckStrict(vkCreateSampler(device.GetDevice(), &createInfo, nullptr, &l_Sampler), "vkCreateSampler");

        return l_Sampler;
    }

    void VulkanResources::DestroySampler(VulkanDevice& device, VkSampler& sampler)
    {
        if (sampler)
        {
            vkDestroySampler(device.GetDevice(), sampler, nullptr);
            sampler = VK_NULL_HANDLE;
        }
    }

    VulkanResources::BufferResource VulkanResources::CreateStagingBuffer(VulkanDevice& device, VkDeviceSize size)
    {
        // Staging buffers live in host visible memory to upload data to the GPU.
        return CreateBuffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void VulkanResources::UploadToBuffer(VulkanDevice& device, VulkanCommand& command, VkBuffer destination, const void* data, VkDeviceSize size)
    {
        // Use a staging buffer so we can upload from host memory to device local memory.
        BufferResource l_Staging = CreateStagingBuffer(device, size);

        void* l_Mapped = nullptr;
        Utilities::VulkanUtilities::VKCheckStrict(vkMapMemory(device.GetDevice(), l_Staging.Memory, 0, size, 0, &l_Mapped), "vkMapMemory (staging buffer)");

        std::memcpy(l_Mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(device.GetDevice(), l_Staging.Memory);

        CopyBuffer(device, command, l_Staging.Buffer, destination, size);

        DestroyBuffer(device, l_Staging);
    }

    void VulkanResources::UploadToImage(VulkanDevice& device, VulkanCommand& command, VkImage image, uint32_t width, uint32_t height, VkFormat format, const void* data, VkDeviceSize size,
        VkImageLayout finalLayout, uint32_t mipLevels, uint32_t arrayLayers)
    {
        // Staging buffer is required because most images reside in device local memory.
        BufferResource l_Staging = CreateStagingBuffer(device, size);

        void* l_Mapped = nullptr;
        Utilities::VulkanUtilities::VKCheckStrict(vkMapMemory(device.GetDevice(), l_Staging.Memory, 0, size, 0, &l_Mapped), "vkMapMemory (staging image)");

        std::memcpy(l_Mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(device.GetDevice(), l_Staging.Memory);

        const VkImageAspectFlags l_AspectFlags = GetAspectFlags(format);

        // Transition to transfer destination so the copy can write into the image.
        TransitionImageLayout(device, command, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, l_AspectFlags, mipLevels, arrayLayers);

        CopyBufferToImage(device, command, l_Staging.Buffer, image, width, height, l_AspectFlags, arrayLayers);

        // Transition to the requested layout for shader access or general usage.
        TransitionImageLayout(device, command, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout, l_AspectFlags, mipLevels, arrayLayers);

        DestroyBuffer(device, l_Staging);
    }

    uint32_t VulkanResources::FindMemoryType(VulkanDevice& device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        // Choose a compatible memory type that supports the desired properties.
        VkPhysicalDeviceMemoryProperties l_MemoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(device.GetPhysicalDevice(), &l_MemoryProperties);

        for (uint32_t i = 0; i < l_MemoryProperties.memoryTypeCount; ++i)
        {
            const bool l_TypeSupported = (typeFilter & (1u << i)) != 0;
            const bool l_HasProperties = (l_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
            if (l_TypeSupported && l_HasProperties)
            {
                return i;
            }
        }

        // If no compatible memory type is found, Vulkan resource creation must fail.
        std::abort();

        return 0u;
    }

    VkImageAspectFlags VulkanResources::GetAspectFlags(VkFormat format)
    {
        // Select the correct aspect mask so transitions and copies target valid planes.
        switch (format)
        {
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            default:
                break;
        }

        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    void VulkanResources::TransitionImageLayout(VulkanDevice& device, VulkanCommand& command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags, 
        uint32_t mipLevels, uint32_t arrayLayers)
    {
        // Layout transitions make image usage explicit and avoid undefined behavior.
        VkCommandBuffer l_Command = command.BeginSingleTime();

        VkImageMemoryBarrier l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        l_Barrier.oldLayout = oldLayout;
        l_Barrier.newLayout = newLayout;
        l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.image = image;
        l_Barrier.subresourceRange.aspectMask = aspectFlags;
        l_Barrier.subresourceRange.baseMipLevel = 0;
        l_Barrier.subresourceRange.levelCount = mipLevels;
        l_Barrier.subresourceRange.baseArrayLayer = 0;
        l_Barrier.subresourceRange.layerCount = arrayLayers;

        VkPipelineStageFlags l_SourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkPipelineStageFlags l_DestinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            // Undefined to transfer destination has no previous access.
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            l_SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            l_DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            // Transfer writes must complete before shader sampling.
            l_Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            l_SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            l_DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
        {
            // General layout is typically used for storage images.
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            l_SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            l_DestinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }
        else
        {
            // Fallback for uncommon transitions keeps behavior explicit but conservative.
            l_Barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            l_SourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            l_DestinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }

        vkCmdPipelineBarrier(l_Command, l_SourceStage, l_DestinationStage, 0, 0, nullptr, 0, nullptr, 1, &l_Barrier);

        command.EndSingleTime(l_Command, device.GetGraphicsQueue());
    }

    void VulkanResources::CopyBuffer(VulkanDevice& device, VulkanCommand& command, VkBuffer source, VkBuffer destination, VkDeviceSize size)
    {
        // Use a one-time command buffer to keep uploads isolated.
        VkCommandBuffer l_Command = command.BeginSingleTime();

        VkBufferCopy l_Copy{};
        l_Copy.srcOffset = 0;
        l_Copy.dstOffset = 0;
        l_Copy.size = size;

        vkCmdCopyBuffer(l_Command, source, destination, 1, &l_Copy);

        command.EndSingleTime(l_Command, device.GetGraphicsQueue());
    }

    void VulkanResources::CopyBufferToImage(VulkanDevice& device, VulkanCommand& command, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkImageAspectFlags aspectFlags, uint32_t arrayLayers)
    {
        // Copy from the staging buffer into the target image.
        VkCommandBuffer l_Command = command.BeginSingleTime();

        VkBufferImageCopy l_Region{};
        l_Region.bufferOffset = 0;
        l_Region.bufferRowLength = 0;
        l_Region.bufferImageHeight = 0;
        l_Region.imageSubresource.aspectMask = aspectFlags;
        l_Region.imageSubresource.mipLevel = 0;
        l_Region.imageSubresource.baseArrayLayer = 0;
        l_Region.imageSubresource.layerCount = arrayLayers;
        l_Region.imageOffset = { 0, 0, 0 };
        l_Region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(l_Command, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_Region);

        command.EndSingleTime(l_Command, device.GetGraphicsQueue());
    }
}