#include "Trinity/Renderer/Vulkan/VulkanUploadQueue.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace Trinity
{
    namespace
    {
        constexpr uint64_t s_DefaultStagingChunkSize = 16ull * 1024ull * 1024ull;
        constexpr uint64_t s_StagingAlignment = 16ull;

        uint64_t AlignUp(uint64_t value, uint64_t alignment)
        {
            return (value + (alignment - 1)) & ~(alignment - 1);
        }
    }

    void VulkanUploadQueue::Initialize(VkDevice device, VmaAllocator allocator, const UploadQueueSpecification& specification)
    {
        TR_CORE_INFO("Initializing Vulkan Upload Queue");

        m_Device = device;
        m_Allocator = allocator;
        m_TransferQueue = specification.TransferQueue;
        m_ConsumerQueue = specification.ConsumerQueue;
        m_TransferQueueFamily = specification.TransferQueueFamily;
        m_ConsumerQueueFamily = specification.ConsumerQueueFamily;
        m_HasDedicatedTransfer = specification.HasDedicatedTransfer;
        m_HasTimelineSemaphore = specification.HasTimelineSemaphore;
        m_RequiresOwnershipTransfer = (m_TransferQueueFamily != m_ConsumerQueueFamily);

        VkCommandPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        l_PoolInfo.queueFamilyIndex = m_TransferQueueFamily;

        VulkanUtilities::VKCheck(vkCreateCommandPool(m_Device, &l_PoolInfo, nullptr, &m_CommandPool), "Failed to create upload command pool");

        if (m_HasTimelineSemaphore)
        {
            VkSemaphoreTypeCreateInfo l_TypeInfo{};
            l_TypeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            l_TypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            l_TypeInfo.initialValue = 0;

            VkSemaphoreCreateInfo l_SemInfo{};
            l_SemInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            l_SemInfo.pNext = &l_TypeInfo;

            VulkanUtilities::VKCheck(vkCreateSemaphore(m_Device, &l_SemInfo, nullptr, &m_TimelineSemaphore), "Failed to create upload timeline semaphore");
        }

        TR_CORE_INFO("Vulkan Upload Queue Initialized (async={}, dedicatedTransfer={}, timelineSem={}, ownershipTransfer={})", IsAsync(), m_HasDedicatedTransfer, m_HasTimelineSemaphore, m_RequiresOwnershipTransfer);
    }

    void VulkanUploadQueue::Shutdown()
    {
        TR_CORE_INFO("Shutting Down Vulkan Upload Queue");

        if (m_HasTimelineSemaphore && m_TimelineSemaphore != VK_NULL_HANDLE && m_LastSubmittedValue > 0)
        {
            VkSemaphoreWaitInfo l_WaitInfo{};
            l_WaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            l_WaitInfo.semaphoreCount = 1;
            l_WaitInfo.pSemaphores = &m_TimelineSemaphore;
            l_WaitInfo.pValues = &m_LastSubmittedValue;
            vkWaitSemaphores(m_Device, &l_WaitInfo, std::numeric_limits<uint64_t>::max());
        }

        if (!m_InFlightCommandBuffers.empty() && m_CommandPool != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_InFlightCommandBuffers.size()), m_InFlightCommandBuffers.data());
            m_InFlightCommandBuffers.clear();
        }

        for (auto& it_Chunk : m_StagingChunks)
        {
            if (it_Chunk.Buffer != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(m_Allocator, it_Chunk.Buffer, it_Chunk.Allocation);
            }
        }
        m_StagingChunks.clear();

        if (m_TimelineSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_Device, m_TimelineSemaphore, nullptr);
            m_TimelineSemaphore = VK_NULL_HANDLE;
        }

        if (m_CommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }

        m_PendingBufferUploads.clear();
        m_PendingTextureUploads.clear();
        m_PendingAcquireBarriers.clear();

        m_Device = VK_NULL_HANDLE;
        m_Allocator = VK_NULL_HANDLE;
        m_TransferQueue = VK_NULL_HANDLE;
        m_ConsumerQueue = VK_NULL_HANDLE;
        m_CurrentChunkIndex = UINT32_MAX;
        m_CurrentChunkOffset = 0;
        m_TimelineValue = 0;
        m_LastSubmittedValue = 0;

        TR_CORE_INFO("Vulkan Upload Queue Shutdown Complete");
    }

    VulkanUploadQueue::StagingChunk* VulkanUploadQueue::AcquireStaging(uint64_t size, uint64_t& outOffset, uint32_t& outChunkIndex)
    {
        if (m_CurrentChunkIndex != UINT32_MAX)
        {
            StagingChunk& l_Current = m_StagingChunks[m_CurrentChunkIndex];
            if (l_Current.SubmissionValue == 0 && (l_Current.Size - m_CurrentChunkOffset) >= size)
            {
                outOffset = m_CurrentChunkOffset;
                outChunkIndex = m_CurrentChunkIndex;
                m_CurrentChunkOffset = AlignUp(m_CurrentChunkOffset + size, s_StagingAlignment);

                return &l_Current;
            }
        }

        uint64_t l_CompletedValue = 0;
        if (m_HasTimelineSemaphore && m_TimelineSemaphore != VK_NULL_HANDLE)
        {
            vkGetSemaphoreCounterValue(m_Device, m_TimelineSemaphore, &l_CompletedValue);
        }

        for (uint32_t l_Index = 0; l_Index < m_StagingChunks.size(); ++l_Index)
        {
            StagingChunk& it_Chunk = m_StagingChunks[l_Index];
            const bool l_Free = (it_Chunk.SubmissionValue == 0) || (it_Chunk.SubmissionValue <= l_CompletedValue);
            if (l_Free && it_Chunk.Size >= size)
            {
                it_Chunk.SubmissionValue = 0;
                m_CurrentChunkIndex = l_Index;
                m_CurrentChunkOffset = AlignUp(size, s_StagingAlignment);
                outOffset = 0;
                outChunkIndex = l_Index;
                return &it_Chunk;
            }
        }

        const uint64_t l_AllocSize = std::max<uint64_t>(s_DefaultStagingChunkSize, size);

        StagingChunk l_NewChunk{};
        l_NewChunk.Size = l_AllocSize;

        VkBufferCreateInfo l_BufferInfo{};
        l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        l_BufferInfo.size = l_AllocSize;
        l_BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        l_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo l_AllocInfo{};
        l_AllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        l_AllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo l_StagingInfo{};
        VkResult l_Result = vmaCreateBuffer(m_Allocator, &l_BufferInfo, &l_AllocInfo, &l_NewChunk.Buffer, &l_NewChunk.Allocation, &l_StagingInfo);
        if (l_Result != VK_SUCCESS)
        {
            TR_CORE_ERROR("Upload Queue: failed to allocate staging chunk of {} bytes (VkResult={})", l_AllocSize, static_cast<int>(l_Result));
            outOffset = 0;
            outChunkIndex = UINT32_MAX;
            return nullptr;
        }
        l_NewChunk.MappedData = l_StagingInfo.pMappedData;

        m_StagingChunks.push_back(l_NewChunk);
        m_CurrentChunkIndex = static_cast<uint32_t>(m_StagingChunks.size() - 1);
        m_CurrentChunkOffset = AlignUp(size, s_StagingAlignment);

        outOffset = 0;
        outChunkIndex = m_CurrentChunkIndex;

        TR_CORE_DEBUG("Upload Queue: allocated staging chunk #{} ({} MB)", m_CurrentChunkIndex, l_AllocSize / (1024 * 1024));

        return &m_StagingChunks.back();
    }

    void VulkanUploadQueue::EnqueueBufferUpload(VkBuffer destination, uint64_t destinationOffset, const void* data, uint64_t size)
    {
        if (size == 0 || data == nullptr || destination == VK_NULL_HANDLE)
        {
            TR_CORE_WARN("Upload Queue: EnqueueBufferUpload called with invalid args (size={}, data={}, dst={})", size, data != nullptr, destination != VK_NULL_HANDLE);
            return;
        }

        std::lock_guard<std::mutex> l_Lock(m_Mutex);

        uint64_t l_Offset = 0;
        uint32_t l_ChunkIndex = UINT32_MAX;
        StagingChunk* a_Chunk = AcquireStaging(size, l_Offset, l_ChunkIndex);
        if (a_Chunk == nullptr)
        {
            return;
        }

        std::memcpy(static_cast<uint8_t*>(a_Chunk->MappedData) + l_Offset, data, static_cast<size_t>(size));
        vmaFlushAllocation(m_Allocator, a_Chunk->Allocation, l_Offset, size);

        PendingBufferUpload l_Op{};
        l_Op.StagingChunkIndex = l_ChunkIndex;
        l_Op.StagingOffset = l_Offset;
        l_Op.Size = size;
        l_Op.Destination = destination;
        l_Op.DestinationOffset = destinationOffset;

        m_PendingBufferUploads.push_back(l_Op);

        TR_CORE_TRACE("Upload Queue: enqueued buffer copy ({} bytes, chunk #{}, offset {})", size, l_ChunkIndex, l_Offset);
    }

    void VulkanUploadQueue::EnqueueTextureUpload(VkImage destination, VkImageAspectFlags aspect, uint32_t mipLevel, uint32_t arrayLayer, uint32_t width, uint32_t height, uint32_t depth, const void* data, uint64_t size)
    {
        if (size == 0 || data == nullptr || destination == VK_NULL_HANDLE)
        {
            TR_CORE_WARN("Upload Queue: EnqueueTextureUpload called with invalid args (size={}, data={}, dst={})", size, data != nullptr, destination != VK_NULL_HANDLE);
            return;
        }

        std::lock_guard<std::mutex> l_Lock(m_Mutex);

        uint64_t l_Offset = 0;
        uint32_t l_ChunkIndex = UINT32_MAX;
        StagingChunk* a_Chunk = AcquireStaging(size, l_Offset, l_ChunkIndex);
        if (a_Chunk == nullptr)
        {
            return;
        }

        std::memcpy(static_cast<uint8_t*>(a_Chunk->MappedData) + l_Offset, data, static_cast<size_t>(size));
        vmaFlushAllocation(m_Allocator, a_Chunk->Allocation, l_Offset, size);

        PendingTextureUpload l_Op{};
        l_Op.StagingChunkIndex = l_ChunkIndex;
        l_Op.StagingOffset = l_Offset;
        l_Op.Destination = destination;
        l_Op.Aspect = aspect;
        l_Op.MipLevel = mipLevel;
        l_Op.ArrayLayer = arrayLayer;
        l_Op.Width = width;
        l_Op.Height = height;
        l_Op.Depth = depth;

        m_PendingTextureUploads.push_back(l_Op);

        TR_CORE_TRACE("Upload Queue: enqueued texture copy ({}x{}x{} mip {} layer {}, {} bytes, chunk #{}, offset {})", width, height, depth, mipLevel, arrayLayer, size, l_ChunkIndex, l_Offset);
    }

    void VulkanUploadQueue::BeginFrame()
    {
        std::lock_guard<std::mutex> l_Lock(m_Mutex);
        RecycleCompletedStagings();
    }

    void VulkanUploadQueue::FlushAsync()
    {
        std::lock_guard<std::mutex> l_Lock(m_Mutex);

        if (m_PendingBufferUploads.empty() && m_PendingTextureUploads.empty())
        {
            return;
        }

        VkCommandBuffer l_Cmd = BeginTransferCommandBuffer();
        RecordCopiesAndBarriers(l_Cmd);
        Submit(l_Cmd, false);
    }

    void VulkanUploadQueue::FlushSync()
    {
        std::lock_guard<std::mutex> l_Lock(m_Mutex);

        if (m_PendingBufferUploads.empty() && m_PendingTextureUploads.empty())
        {
            return;
        }

        VkCommandBuffer l_Cmd = BeginTransferCommandBuffer();
        RecordCopiesAndBarriers(l_Cmd);
        Submit(l_Cmd, true);
    }

    VkCommandBuffer VulkanUploadQueue::BeginTransferCommandBuffer()
    {
        VkCommandBufferAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocInfo.commandPool = m_CommandPool;
        l_AllocInfo.commandBufferCount = 1;

        VkCommandBuffer l_Cmd = VK_NULL_HANDLE;
        VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_Device, &l_AllocInfo, &l_Cmd), "Failed to allocate upload command buffer");

        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VulkanUtilities::VKCheck(vkBeginCommandBuffer(l_Cmd, &l_BeginInfo), "Failed to begin upload command buffer");

        return l_Cmd;
    }

    void VulkanUploadQueue::RecordCopiesAndBarriers(VkCommandBuffer commandBuffer)
    {
        std::vector<VkImage> l_UniqueImages;
        std::vector<VkImageAspectFlags> l_UniqueAspects;
        for (const auto& it_Op : m_PendingTextureUploads)
        {
            auto a_Existing = std::find(l_UniqueImages.begin(), l_UniqueImages.end(), it_Op.Destination);
            if (a_Existing == l_UniqueImages.end())
            {
                l_UniqueImages.push_back(it_Op.Destination);
                l_UniqueAspects.push_back(it_Op.Aspect);
            }
        }

        if (!l_UniqueImages.empty())
        {
            std::vector<VkImageMemoryBarrier2> l_Barriers;
            l_Barriers.reserve(l_UniqueImages.size());

            for (size_t l_Index = 0; l_Index < l_UniqueImages.size(); ++l_Index)
            {
                VkImageMemoryBarrier2 l_Barrier{};
                l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                l_Barrier.srcAccessMask = 0;
                l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
                l_Barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                l_Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                l_Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                l_Barrier.image = l_UniqueImages[l_Index];
                l_Barrier.subresourceRange.aspectMask = l_UniqueAspects[l_Index];
                l_Barrier.subresourceRange.baseMipLevel = 0;
                l_Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                l_Barrier.subresourceRange.baseArrayLayer = 0;
                l_Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
                l_Barriers.push_back(l_Barrier);
            }

            VkDependencyInfo l_DepInfo{};
            l_DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            l_DepInfo.imageMemoryBarrierCount = static_cast<uint32_t>(l_Barriers.size());
            l_DepInfo.pImageMemoryBarriers = l_Barriers.data();
            vkCmdPipelineBarrier2(commandBuffer, &l_DepInfo);
        }

        for (const auto& it_Op : m_PendingBufferUploads)
        {
            VkBufferCopy l_Copy{};
            l_Copy.srcOffset = it_Op.StagingOffset;
            l_Copy.dstOffset = it_Op.DestinationOffset;
            l_Copy.size = it_Op.Size;
            vkCmdCopyBuffer(commandBuffer, m_StagingChunks[it_Op.StagingChunkIndex].Buffer, it_Op.Destination, 1, &l_Copy);
        }

        for (const auto& it_Op : m_PendingTextureUploads)
        {
            VkBufferImageCopy l_Copy{};
            l_Copy.bufferOffset = it_Op.StagingOffset;
            l_Copy.bufferRowLength = 0;
            l_Copy.bufferImageHeight = 0;
            l_Copy.imageSubresource.aspectMask = it_Op.Aspect;
            l_Copy.imageSubresource.mipLevel = it_Op.MipLevel;
            l_Copy.imageSubresource.baseArrayLayer = it_Op.ArrayLayer;
            l_Copy.imageSubresource.layerCount = 1;
            l_Copy.imageOffset = { 0, 0, 0 };
            l_Copy.imageExtent = { it_Op.Width, it_Op.Height, it_Op.Depth };

            vkCmdCopyBufferToImage(commandBuffer, m_StagingChunks[it_Op.StagingChunkIndex].Buffer, it_Op.Destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_Copy);
        }

        if (!l_UniqueImages.empty())
        {
            std::vector<VkImageMemoryBarrier2> l_ReleaseBarriers;
            l_ReleaseBarriers.reserve(l_UniqueImages.size());

            for (size_t l_Index = 0; l_Index < l_UniqueImages.size(); ++l_Index)
            {
                VkImageMemoryBarrier2 l_Barrier{};
                l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
                l_Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

                if (m_RequiresOwnershipTransfer)
                {
                    l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
                    l_Barrier.dstAccessMask = 0;
                    l_Barrier.srcQueueFamilyIndex = m_TransferQueueFamily;
                    l_Barrier.dstQueueFamilyIndex = m_ConsumerQueueFamily;
                }
                else
                {
                    l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                    l_Barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                    l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                }

                l_Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                l_Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                l_Barrier.image = l_UniqueImages[l_Index];
                l_Barrier.subresourceRange.aspectMask = l_UniqueAspects[l_Index];
                l_Barrier.subresourceRange.baseMipLevel = 0;
                l_Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                l_Barrier.subresourceRange.baseArrayLayer = 0;
                l_Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
                l_ReleaseBarriers.push_back(l_Barrier);

                if (m_RequiresOwnershipTransfer)
                {
                    PendingAcquireBarrier l_Acquire{};
                    l_Acquire.Image = l_UniqueImages[l_Index];
                    l_Acquire.Aspect = l_UniqueAspects[l_Index];
                    m_PendingAcquireBarriers.push_back(l_Acquire);
                }
            }

            VkDependencyInfo l_DepInfo{};
            l_DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            l_DepInfo.imageMemoryBarrierCount = static_cast<uint32_t>(l_ReleaseBarriers.size());
            l_DepInfo.pImageMemoryBarriers = l_ReleaseBarriers.data();
            vkCmdPipelineBarrier2(commandBuffer, &l_DepInfo);
        }
    }

    void VulkanUploadQueue::Submit(VkCommandBuffer commandBuffer, bool waitSync)
    {
        VulkanUtilities::VKCheck(vkEndCommandBuffer(commandBuffer), "Failed to end upload command buffer");

        const uint64_t l_SignalValue = ++m_TimelineValue;

        if (m_CurrentChunkIndex != UINT32_MAX)
        {
            m_StagingChunks[m_CurrentChunkIndex].SubmissionValue = l_SignalValue;
        }

        const uint32_t l_BufferOps = static_cast<uint32_t>(m_PendingBufferUploads.size());
        const uint32_t l_TextureOps = static_cast<uint32_t>(m_PendingTextureUploads.size());

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &commandBuffer;

        VkTimelineSemaphoreSubmitInfo l_TimelineInfo{};
        VkFence l_SubmitFence = VK_NULL_HANDLE;

        if (m_HasTimelineSemaphore)
        {
            l_TimelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
            l_TimelineInfo.signalSemaphoreValueCount = 1;
            l_TimelineInfo.pSignalSemaphoreValues = &l_SignalValue;

            l_SubmitInfo.signalSemaphoreCount = 1;
            l_SubmitInfo.pSignalSemaphores = &m_TimelineSemaphore;
            l_SubmitInfo.pNext = &l_TimelineInfo;
        }
        else
        {
            VkFenceCreateInfo l_FenceInfo{};
            l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            VulkanUtilities::VKCheck(vkCreateFence(m_Device, &l_FenceInfo, nullptr, &l_SubmitFence), "Failed to create upload fence");
        }

        VulkanUtilities::VKCheck(vkQueueSubmit(m_TransferQueue, 1, &l_SubmitInfo, l_SubmitFence), "Failed to submit upload queue work");

        m_LastSubmittedValue = l_SignalValue;
        m_InFlightCommandBuffers.push_back(commandBuffer);

        if (waitSync)
        {
            if (m_HasTimelineSemaphore)
            {
                VkSemaphoreWaitInfo l_WaitInfo{};
                l_WaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
                l_WaitInfo.semaphoreCount = 1;
                l_WaitInfo.pSemaphores = &m_TimelineSemaphore;
                l_WaitInfo.pValues = &l_SignalValue;
                vkWaitSemaphores(m_Device, &l_WaitInfo, std::numeric_limits<uint64_t>::max());
            }
            else if (l_SubmitFence != VK_NULL_HANDLE)
            {
                vkWaitForFences(m_Device, 1, &l_SubmitFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
            }
        }

        if (l_SubmitFence != VK_NULL_HANDLE)
        {
            vkDestroyFence(m_Device, l_SubmitFence, nullptr);
        }

        m_PendingBufferUploads.clear();
        m_PendingTextureUploads.clear();

        TR_CORE_DEBUG("Upload Queue: submitted batch (value={}, buffers={}, textures={}, sync={})", l_SignalValue, l_BufferOps, l_TextureOps, waitSync);
    }

    void VulkanUploadQueue::RecordAcquireBarriers(VkCommandBuffer consumerCommandBuffer)
    {
        std::lock_guard<std::mutex> l_Lock(m_Mutex);

        if (m_PendingAcquireBarriers.empty())
        {
            return;
        }

        if (!m_RequiresOwnershipTransfer)
        {
            m_PendingAcquireBarriers.clear();
            return;
        }

        std::vector<VkImageMemoryBarrier2> l_Barriers;
        l_Barriers.reserve(m_PendingAcquireBarriers.size());

        for (const auto& it_Acquire : m_PendingAcquireBarriers)
        {
            VkImageMemoryBarrier2 l_Barrier{};
            l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            l_Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            l_Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            l_Barrier.srcQueueFamilyIndex = m_TransferQueueFamily;
            l_Barrier.dstQueueFamilyIndex = m_ConsumerQueueFamily;
            l_Barrier.image = it_Acquire.Image;
            l_Barrier.subresourceRange.aspectMask = it_Acquire.Aspect;
            l_Barrier.subresourceRange.baseMipLevel = 0;
            l_Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            l_Barrier.subresourceRange.baseArrayLayer = 0;
            l_Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            l_Barriers.push_back(l_Barrier);
        }

        VkDependencyInfo l_DepInfo{};
        l_DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_DepInfo.imageMemoryBarrierCount = static_cast<uint32_t>(l_Barriers.size());
        l_DepInfo.pImageMemoryBarriers = l_Barriers.data();
        vkCmdPipelineBarrier2(consumerCommandBuffer, &l_DepInfo);

        TR_CORE_TRACE("Upload Queue: recorded {} acquire barriers", m_PendingAcquireBarriers.size());

        m_PendingAcquireBarriers.clear();
    }

    void VulkanUploadQueue::RecycleCompletedStagings()
    {
        if (!m_HasTimelineSemaphore || m_TimelineSemaphore == VK_NULL_HANDLE)
        {
            return;
        }

        if (m_InFlightCommandBuffers.empty())
        {
            return;
        }

        uint64_t l_Completed = 0;
        vkGetSemaphoreCounterValue(m_Device, m_TimelineSemaphore, &l_Completed);

        if (l_Completed >= m_LastSubmittedValue && m_LastSubmittedValue > 0)
        {
            vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_InFlightCommandBuffers.size()), m_InFlightCommandBuffers.data());
            m_InFlightCommandBuffers.clear();
        }
    }

    bool VulkanUploadQueue::HasPendingWaitInfo() const
    {
        return m_HasTimelineSemaphore && m_HasDedicatedTransfer && m_LastSubmittedValue > 0;
    }
}