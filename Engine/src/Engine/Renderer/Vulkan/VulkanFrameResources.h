#pragma once

#include "Engine/Renderer/Vulkan/VulkanCommand.h"
#include "Engine/Renderer/Vulkan/VulkanDescriptors.h"
#include "Engine/Renderer/Vulkan/VulkanSync.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Engine
{
    class VulkanDevice;

    class VulkanFrameResources
    {
    public:
        VulkanFrameResources() = default;
        ~VulkanFrameResources() = default;

        VulkanFrameResources(const VulkanFrameResources&) = delete;
        VulkanFrameResources& operator=(const VulkanFrameResources&) = delete;
        VulkanFrameResources(VulkanFrameResources&&) = delete;
        VulkanFrameResources& operator=(VulkanFrameResources&&) = delete;

        void Initialize(VulkanDevice& device, uint32_t framesInFlight);
        void Shutdown(VulkanDevice& device);

        void OnBeginFrame(VulkanDevice& device, uint32_t frameIndex);
        void ResetForFrame(VulkanDevice& device, uint32_t frameIndex);
        void OnSwapchainRecreated(size_t swapchainImageCount);

        bool IsValid() const;
        bool HasDescriptors() const;

        VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const;
        VkSemaphore GetImageAvailableSemaphore(uint32_t frameIndex) const;
        VkSemaphore GetRenderFinishedSemaphore(uint32_t frameIndex) const;
        VkFence GetInFlightFence(uint32_t frameIndex) const;
        VkDescriptorSetLayout GetGlobalDescriptorSetLayout() const;
        VkDescriptorSetLayout GetMaterialDescriptorSetLayout() const;
        VkDescriptorSet AllocateGlobalDescriptorSet(uint32_t frameIndex) const;
        VkDescriptorSet AllocateMaterialDescriptorSet(uint32_t frameIndex) const;
        uint32_t GetFramesInFlight() const;

        void WaitForFrameFence(VulkanDevice& device, uint32_t frameIndex, uint64_t timeout = UINT64_MAX) const;
        void WaitForImageFenceIfNeeded(VulkanDevice& device, uint32_t imageIndex, uint64_t timeout = UINT64_MAX) const;
        void MarkImageInFlight(uint32_t imageIndex, VkFence owningFrameFence);

    private:
        VulkanCommand m_Command;
        VulkanSync m_Sync;
        VulkanDescriptors m_Descriptors;

        uint32_t m_FramesInFlight = 0;
        bool m_Initialized = false;
    };
}