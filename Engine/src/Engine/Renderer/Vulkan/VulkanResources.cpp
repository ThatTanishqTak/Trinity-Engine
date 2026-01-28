#include "Engine/Renderer/Vulkan/VulkanResources.h"

#include "Engine/Utilities/Utilities.h"

#include <cstring>
#include <cstdlib>

namespace Engine
{
    void VulkanResources::Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool)
    {
        m_PhysicalDevice = physicalDevice;
        m_Device = device;
        m_GraphicsQueue = graphicsQueue;
        m_CommandPool = commandPool;
    }

    void VulkanResources::Shutdown()
    {
        m_PhysicalDevice = VK_NULL_HANDLE;
        m_Device = VK_NULL_HANDLE;
        m_GraphicsQueue = VK_NULL_HANDLE;
        m_CommandPool = VK_NULL_HANDLE;
    }

    uint32_t VulkanResources::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDeviceMemoryProperties l_MemProps{};
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &l_MemProps);

        for (uint32_t i = 0; i < l_MemProps.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1u << i)) && (l_MemProps.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        TR_CORE_CRITICAL("VulkanResources::FindMemoryType failed. No suitable memory type found.");

        std::abort();
    }

    void VulkanResources::CreateBuffer(VkDeviceSize sizeBytes, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& outBuffer, VkDeviceMemory& outMemory) const
    {
        outBuffer = VK_NULL_HANDLE;
        outMemory = VK_NULL_HANDLE;

        VkBufferCreateInfo l_BufferInfo{};
        l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        l_BufferInfo.size = sizeBytes;
        l_BufferInfo.usage = usage;
        l_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateBuffer(m_Device, &l_BufferInfo, nullptr, &outBuffer), "vkCreateBuffer");

        VkMemoryRequirements l_MemReq{};
        vkGetBufferMemoryRequirements(m_Device, outBuffer, &l_MemReq);

        VkMemoryAllocateInfo l_Alloc{};
        l_Alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        l_Alloc.allocationSize = l_MemReq.size;
        l_Alloc.memoryTypeIndex = FindMemoryType(l_MemReq.memoryTypeBits, properties);

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkAllocateMemory(m_Device, &l_Alloc, nullptr, &outMemory), "vkAllocateMemory(buffer)");
        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkBindBufferMemory(m_Device, outBuffer, outMemory, 0), "vkBindBufferMemory");
    }

    void VulkanResources::DestroyBuffer(VkBuffer& buffer, VkDeviceMemory& memory) const
    {
        if (buffer)
        {
            vkDestroyBuffer(m_Device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }

        if (memory)
        {
            vkFreeMemory(m_Device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }

    VkCommandBuffer VulkanResources::BeginSingleTimeCommands() const
    {
        VkCommandBufferAllocateInfo l_Alloc{};
        l_Alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_Alloc.commandPool = m_CommandPool;
        l_Alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_Alloc.commandBufferCount = 1;

        VkCommandBuffer l_CommandBuffer = VK_NULL_HANDLE;
        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkAllocateCommandBuffers(m_Device, &l_Alloc, &l_CommandBuffer), "vkAllocateCommandBuffers(single time)");

        VkCommandBufferBeginInfo l_Begin{};
        l_Begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_Begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkBeginCommandBuffer(l_CommandBuffer, &l_Begin), "vkBeginCommandBuffer(single time)");

        return l_CommandBuffer;
    }

    void VulkanResources::EndSingleTimeCommands(VkCommandBuffer commandBuffer) const
    {
        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer(single time)");

        VkSubmitInfo l_Submit{};
        l_Submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_Submit.commandBufferCount = 1;
        l_Submit.pCommandBuffers = &commandBuffer;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkQueueSubmit(m_GraphicsQueue, 1, &l_Submit, VK_NULL_HANDLE), "vkQueueSubmit(single time)");

        vkQueueWaitIdle(m_GraphicsQueue);

        vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
    }

    void VulkanResources::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize sizeBytes) const
    {
        VkCommandBuffer l_Cmd = BeginSingleTimeCommands();

        VkBufferCopy l_Copy{};
        l_Copy.size = sizeBytes;
        vkCmdCopyBuffer(l_Cmd, srcBuffer, dstBuffer, 1, &l_Copy);

        EndSingleTimeCommands(l_Cmd);
    }

    void VulkanResources::CreateVertexBufferStaged(const void* vertexData, VkDeviceSize sizeBytes, VkBuffer& outBuffer, VkDeviceMemory& outMemory) const
    {
        VkBuffer l_StagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory l_StagingMemory = VK_NULL_HANDLE;

        CreateBuffer(sizeBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, l_StagingBuffer, l_StagingMemory);

        void* l_Mapped = nullptr;
        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkMapMemory(m_Device, l_StagingMemory, 0, sizeBytes, 0, &l_Mapped), "vkMapMemory(staging)");

        std::memcpy(l_Mapped, vertexData, static_cast<size_t>(sizeBytes));
        vkUnmapMemory(m_Device, l_StagingMemory);

        CreateBuffer(sizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory);

        CopyBuffer(l_StagingBuffer, outBuffer, sizeBytes);

        DestroyBuffer(l_StagingBuffer, l_StagingMemory);
    }

    void VulkanResources::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& outImage, VkDeviceMemory& outMemory) const
    {
        outImage = VK_NULL_HANDLE;
        outMemory = VK_NULL_HANDLE;

        VkImageCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        l_Info.imageType = VK_IMAGE_TYPE_2D;
        l_Info.extent.width = width;
        l_Info.extent.height = height;
        l_Info.extent.depth = 1;
        l_Info.mipLevels = 1;
        l_Info.arrayLayers = 1;
        l_Info.format = format;
        l_Info.tiling = tiling;
        l_Info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        l_Info.usage = usage;
        l_Info.samples = VK_SAMPLE_COUNT_1_BIT;
        l_Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateImage(m_Device, &l_Info, nullptr, &outImage), "vkCreateImage");

        VkMemoryRequirements l_MemReq{};
        vkGetImageMemoryRequirements(m_Device, outImage, &l_MemReq);

        VkMemoryAllocateInfo l_Alloc{};
        l_Alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        l_Alloc.allocationSize = l_MemReq.size;
        l_Alloc.memoryTypeIndex = FindMemoryType(l_MemReq.memoryTypeBits, properties);

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkAllocateMemory(m_Device, &l_Alloc, nullptr, &outMemory), "vkAllocateMemory(image)");
        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkBindImageMemory(m_Device, outImage, outMemory, 0), "vkBindImageMemory");
    }

    VkImageView VulkanResources::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const
    {
        VkImageViewCreateInfo l_View{};
        l_View.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_View.image = image;
        l_View.viewType = VK_IMAGE_VIEW_TYPE_2D;
        l_View.format = format;
        l_View.subresourceRange.aspectMask = aspectFlags;
        l_View.subresourceRange.baseMipLevel = 0;
        l_View.subresourceRange.levelCount = 1;
        l_View.subresourceRange.baseArrayLayer = 0;
        l_View.subresourceRange.layerCount = 1;

        VkImageView l_ImageView = VK_NULL_HANDLE;
        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateImageView(m_Device, &l_View, nullptr, &l_ImageView), "vkCreateImageView");

        return l_ImageView;
    }

    void VulkanResources::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags) const
    {
        VkCommandBuffer l_Cmd = BeginSingleTimeCommands();

        VkImageMemoryBarrier l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        l_Barrier.oldLayout = oldLayout;
        l_Barrier.newLayout = newLayout;
        l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.image = image;
        l_Barrier.subresourceRange.aspectMask = aspectFlags;
        l_Barrier.subresourceRange.baseMipLevel = 0;
        l_Barrier.subresourceRange.levelCount = 1;
        l_Barrier.subresourceRange.baseArrayLayer = 0;
        l_Barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags l_SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags l_DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            l_SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            l_DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            l_Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            l_SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            l_DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            TR_CORE_CRITICAL("VulkanResources::TransitionImageLayout unsupported transition.");

            std::abort();
        }

        vkCmdPipelineBarrier(l_Cmd, l_SrcStage, l_DstStage, 0, 0, nullptr, 0, nullptr, 1, &l_Barrier);

        EndSingleTimeCommands(l_Cmd);
    }

    void VulkanResources::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const
    {
        VkCommandBuffer l_Cmd = BeginSingleTimeCommands();

        VkBufferImageCopy l_Region{};
        l_Region.bufferOffset = 0;
        l_Region.bufferRowLength = 0;
        l_Region.bufferImageHeight = 0;

        l_Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        l_Region.imageSubresource.mipLevel = 0;
        l_Region.imageSubresource.baseArrayLayer = 0;
        l_Region.imageSubresource.layerCount = 1;

        l_Region.imageOffset = { 0, 0, 0 };
        l_Region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(l_Cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_Region);

        EndSingleTimeCommands(l_Cmd);
    }
}