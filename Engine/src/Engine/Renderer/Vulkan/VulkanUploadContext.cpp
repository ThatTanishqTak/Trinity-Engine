#include "Engine/Renderer/Vulkan/VulkanUploadContext.h"

#include "Engine/Renderer/Vulkan/VulkanDebugUtils.h"
#include "Engine/Renderer/Vulkan/VulkanDevice.h"
#include "Engine/Utilities/Utilities.h"

#include <cstring>

namespace Engine
{
    void VulkanUploadContext::Initialize(VulkanDevice& device)
    {
        m_Device = &device;

        VkCommandPoolCreateInfo l_CommandPoolCreateInfo{};
        l_CommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        l_CommandPoolCreateInfo.queueFamilyIndex = device.GetGraphicsQueueFamily();

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateCommandPool(device.GetDevice(), &l_CommandPoolCreateInfo, nullptr, &m_CommandPool), "vkCreateCommandPool (upload)");
        const VulkanDebugUtils* l_DebugUtils = device.GetDebugUtils();
        if (l_DebugUtils && m_CommandPool)
        {
            l_DebugUtils->SetObjectName(device.GetDevice(), VK_OBJECT_TYPE_COMMAND_POOL, static_cast<uint64_t>(m_CommandPool), "Upload_CommandPool");
        }


        VkCommandBufferAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocateInfo.commandPool = m_CommandPool;
        l_AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocateInfo.commandBufferCount = 1;

        Utilities::VulkanUtilities::VKCheckStrict(vkAllocateCommandBuffers(device.GetDevice(), &l_AllocateInfo, &m_CommandBuffer), "vkAllocateCommandBuffers (upload)");
        if (l_DebugUtils && m_CommandBuffer)
        {
            l_DebugUtils->SetObjectName(device.GetDevice(), VK_OBJECT_TYPE_COMMAND_BUFFER, static_cast<uint64_t>(m_CommandBuffer), "Upload_CommandBuffer");
        }

        VkFenceCreateInfo l_FenceCreateInfo{};
        l_FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        l_FenceCreateInfo.flags = 0;

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateFence(device.GetDevice(), &l_FenceCreateInfo, nullptr, &m_UploadFence), "vkCreateFence (upload)");
        if (l_DebugUtils && m_UploadFence)
        {
            l_DebugUtils->SetObjectName(device.GetDevice(), VK_OBJECT_TYPE_FENCE, static_cast<uint64_t>(m_UploadFence), "Upload_Fence");
        }
    }

    void VulkanUploadContext::Shutdown(VulkanDevice& device)
    {
        if (m_UploadFence)
        {
            vkDestroyFence(device.GetDevice(), m_UploadFence, nullptr);
            m_UploadFence = VK_NULL_HANDLE;
        }

        if (m_CommandBuffer)
        {
            vkFreeCommandBuffers(device.GetDevice(), m_CommandPool, 1, &m_CommandBuffer);
            m_CommandBuffer = VK_NULL_HANDLE;
        }

        if (m_CommandPool)
        {
            vkDestroyCommandPool(device.GetDevice(), m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }

        m_Device = nullptr;
    }

    void VulkanUploadContext::Begin()
    {
        if (!m_Device || !m_Device->GetDevice() || !m_CommandPool || !m_CommandBuffer || !m_UploadFence)
        {
            return;
        }

        Utilities::VulkanUtilities::VKCheckStrict(vkResetFences(m_Device->GetDevice(), 1, &m_UploadFence), "vkResetFences (upload)");
        Utilities::VulkanUtilities::VKCheckStrict(vkResetCommandPool(m_Device->GetDevice(), m_CommandPool, 0), "vkResetCommandPool (upload)");

        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        Utilities::VulkanUtilities::VKCheckStrict(vkBeginCommandBuffer(m_CommandBuffer, &l_BeginInfo), "vkBeginCommandBuffer (upload)");
    }

    void VulkanUploadContext::EndAndSubmitAndWait()
    {
        if (!m_Device || !m_Device->GetDevice() || !m_CommandBuffer || !m_UploadFence)
        {
            return;
        }

        Utilities::VulkanUtilities::VKCheckStrict(vkEndCommandBuffer(m_CommandBuffer), "vkEndCommandBuffer (upload)");

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &m_CommandBuffer;

        Utilities::VulkanUtilities::VKCheckStrict(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &l_SubmitInfo, m_UploadFence), "vkQueueSubmit (upload)");
        Utilities::VulkanUtilities::VKCheckStrict(vkWaitForFences(m_Device->GetDevice(), 1, &m_UploadFence, VK_TRUE, UINT64_MAX), "vkWaitForFences (upload)");
    }

    void VulkanUploadContext::UploadBuffer(VkBuffer destination, const void* data, VkDeviceSize size)
    {
        if (!m_Device || !m_Device->GetDevice() || !destination || !data || size == 0)
        {
            return;
        }

        TR_CORE_INFO("Uploading buffer {} bytes", static_cast<uint64_t>(size));

        VulkanResources::BufferResource l_Staging = VulkanResources::CreateStagingBuffer(*m_Device, size);

        void* l_Mapped = nullptr;
        Utilities::VulkanUtilities::VKCheckStrict(vkMapMemory(m_Device->GetDevice(), l_Staging.Memory, 0, size, 0, &l_Mapped), "vkMapMemory (upload buffer)");

        std::memcpy(l_Mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(m_Device->GetDevice(), l_Staging.Memory);

        Begin();

        VkBufferCopy l_CopyRegion{};
        l_CopyRegion.size = size;

        vkCmdCopyBuffer(m_CommandBuffer, l_Staging.Buffer, destination, 1, &l_CopyRegion);

        EndAndSubmitAndWait();

        VulkanResources::DestroyBuffer(*m_Device, l_Staging);
    }

    void VulkanUploadContext::UploadImage(VkImage image, uint32_t width, uint32_t height, VkFormat format, const void* data, VkDeviceSize size, VkImageLayout finalLayout, 
        uint32_t mipLevels, uint32_t arrayLayers)
    {
        if (!m_Device || !m_Device->GetDevice() || !image || !data || size == 0)
        {
            return;
        }

        TR_CORE_INFO("Uploading image {}x{} ({} bytes)", width, height, static_cast<uint64_t>(size));

        VulkanResources::BufferResource l_Staging = VulkanResources::CreateStagingBuffer(*m_Device, size);

        void* l_Mapped = nullptr;
        Utilities::VulkanUtilities::VKCheckStrict(vkMapMemory(m_Device->GetDevice(), l_Staging.Memory, 0, size, 0, &l_Mapped), "vkMapMemory (upload image)");

        std::memcpy(l_Mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(m_Device->GetDevice(), l_Staging.Memory);

        const VkImageAspectFlags l_AspectFlags = GetAspectFlags(format);

        Begin();
        
        RecordImageLayoutTransition(m_CommandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, l_AspectFlags, mipLevels, arrayLayers);
        CopyBufferToImage(l_Staging.Buffer, image, width, height, l_AspectFlags, arrayLayers);
        RecordImageLayoutTransition(m_CommandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout, l_AspectFlags, mipLevels, arrayLayers);

        EndAndSubmitAndWait();

        VulkanResources::DestroyBuffer(*m_Device, l_Staging);
    }

    void VulkanUploadContext::RecordImageLayoutTransition(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags,
        uint32_t mipLevels, uint32_t arrayLayers) const
    {
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

        VkPipelineStageFlags l_SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags l_DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            l_SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            l_DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            l_Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            l_SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            l_DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            l_Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            l_SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            l_DestinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }

        vkCmdPipelineBarrier(commandBuffer, l_SourceStage, l_DestinationStage, 0, 0, nullptr, 0, nullptr, 1, &l_Barrier);
    }

    void VulkanUploadContext::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags,
        uint32_t mipLevels, uint32_t arrayLayers) const
    {
        RecordImageLayoutTransition(m_CommandBuffer, image, oldLayout, newLayout, aspectFlags, mipLevels, arrayLayers);
    }

    void VulkanUploadContext::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkImageAspectFlags aspectFlags, uint32_t arrayLayers) const
    {
        VkBufferImageCopy l_CopyRegion{};
        l_CopyRegion.bufferOffset = 0;
        l_CopyRegion.bufferRowLength = 0;
        l_CopyRegion.bufferImageHeight = 0;
        l_CopyRegion.imageSubresource.aspectMask = aspectFlags;
        l_CopyRegion.imageSubresource.mipLevel = 0;
        l_CopyRegion.imageSubresource.baseArrayLayer = 0;
        l_CopyRegion.imageSubresource.layerCount = arrayLayers;
        l_CopyRegion.imageOffset = { 0, 0, 0 };
        l_CopyRegion.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(m_CommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_CopyRegion);
    }

    VkImageAspectFlags VulkanUploadContext::GetAspectFlags(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D32_SFLOAT:
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            case VK_FORMAT_S8_UINT:
                return VK_IMAGE_ASPECT_STENCIL_BIT;
            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            default:
                return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }
}