#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <mutex>
#include <vector>

namespace Trinity
{
    struct UploadQueueSpecification
    {
        VkQueue TransferQueue = VK_NULL_HANDLE;
        VkQueue ConsumerQueue = VK_NULL_HANDLE;
        uint32_t TransferQueueFamily = 0;
        uint32_t ConsumerQueueFamily = 0;
        bool HasDedicatedTransfer = false;
        bool HasTimelineSemaphore = false;
    };

    class VulkanUploadQueue
    {
    public:
        VulkanUploadQueue() = default;
        ~VulkanUploadQueue() = default;

        VulkanUploadQueue(const VulkanUploadQueue&) = delete;
        VulkanUploadQueue& operator=(const VulkanUploadQueue&) = delete;

        void Initialize(VkDevice device, VmaAllocator allocator, const UploadQueueSpecification& specification);
        void Shutdown();

        void EnqueueBufferUpload(VkBuffer destination, uint64_t destinationOffset, const void* data, uint64_t size);
        void EnqueueTextureUpload(VkImage destination, VkImageAspectFlags aspect, uint32_t mipLevel, uint32_t arrayLayer, uint32_t width, uint32_t height, uint32_t depth, const void* data, uint64_t size);

        void BeginFrame();
        void FlushAsync();
        void FlushSync();

        void RecordAcquireBarriers(VkCommandBuffer consumerCommandBuffer);

        bool HasPendingWaitInfo() const;
        VkSemaphore GetTimelineSemaphore() const { return m_TimelineSemaphore; }
        uint64_t GetLastSubmittedValue() const { return m_LastSubmittedValue; }

        bool IsInitialized() const { return m_Device != VK_NULL_HANDLE; }
        bool IsAsync() const { return m_HasDedicatedTransfer && m_HasTimelineSemaphore; }

    private:
        struct StagingChunk
        {
            VkBuffer Buffer = VK_NULL_HANDLE;
            VmaAllocation Allocation = VK_NULL_HANDLE;
            void* MappedData = nullptr;
            uint64_t Size = 0;
            uint64_t SubmissionValue = 0;
        };

        struct PendingBufferUpload
        {
            uint32_t StagingChunkIndex = 0;
            uint64_t StagingOffset = 0;
            uint64_t Size = 0;
            VkBuffer Destination = VK_NULL_HANDLE;
            uint64_t DestinationOffset = 0;
        };

        struct PendingTextureUpload
        {
            uint32_t StagingChunkIndex = 0;
            uint64_t StagingOffset = 0;
            VkImage Destination = VK_NULL_HANDLE;
            VkImageAspectFlags Aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            uint32_t MipLevel = 0;
            uint32_t ArrayLayer = 0;
            uint32_t Width = 0;
            uint32_t Height = 0;
            uint32_t Depth = 1;
        };

        struct PendingAcquireBarrier
        {
            VkImage Image = VK_NULL_HANDLE;
            VkImageAspectFlags Aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        };

        StagingChunk* AcquireStaging(uint64_t size, uint64_t& outOffset, uint32_t& outChunkIndex);
        void RecycleCompletedStagings();
        VkCommandBuffer BeginTransferCommandBuffer();
        void RecordCopiesAndBarriers(VkCommandBuffer commandBuffer);
        void Submit(VkCommandBuffer commandBuffer, bool waitSync);

        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;

        VkQueue m_TransferQueue = VK_NULL_HANDLE;
        VkQueue m_ConsumerQueue = VK_NULL_HANDLE;
        uint32_t m_TransferQueueFamily = 0;
        uint32_t m_ConsumerQueueFamily = 0;
        bool m_HasDedicatedTransfer = false;
        bool m_HasTimelineSemaphore = false;
        bool m_RequiresOwnershipTransfer = false;

        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkSemaphore m_TimelineSemaphore = VK_NULL_HANDLE;
        uint64_t m_TimelineValue = 0;
        uint64_t m_LastSubmittedValue = 0;

        std::vector<StagingChunk> m_StagingChunks;
        uint32_t m_CurrentChunkIndex = UINT32_MAX;
        uint64_t m_CurrentChunkOffset = 0;

        std::vector<PendingBufferUpload> m_PendingBufferUploads;
        std::vector<PendingTextureUpload> m_PendingTextureUploads;
        std::vector<PendingAcquireBarrier> m_PendingAcquireBarriers;

        std::vector<VkCommandBuffer> m_InFlightCommandBuffers;

        std::mutex m_Mutex;
    };
}