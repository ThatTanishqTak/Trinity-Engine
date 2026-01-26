#include "Engine/Renderer/Vulkan/VulkanFrameResources.h"

#include "Engine/Renderer/Vulkan/VulkanDevice.h"

namespace Engine
{
    void VulkanFrameResources::Initialize(VulkanDevice& device, uint32_t framesInFlight)
    {
        Shutdown(device);

        m_FramesInFlight = framesInFlight;

        // Command buffers, fences, semaphores, and descriptor sets all advance per frame in flight.
        m_Command.Initialize(device, m_FramesInFlight);
        m_Sync.Initialize(device, m_FramesInFlight, 0);
        m_Descriptors.Initialize(device, m_FramesInFlight);

        m_Initialized = true;
    }

    void VulkanFrameResources::Shutdown(VulkanDevice& device)
    {
        if (!m_Initialized)
        {
            m_FramesInFlight = 0;
            return;
        }

        m_Descriptors.Shutdown(device);
        m_Sync.Shutdown(device);
        m_Command.Shutdown();

        m_FramesInFlight = 0;
        m_Initialized = false;
    }

    void VulkanFrameResources::ResetForFrame(VulkanDevice& device, uint32_t frameIndex)
    {
        m_Sync.ResetFrameFence(device, frameIndex);
        m_Command.ResetCommandBuffer(frameIndex);
    }

    void VulkanFrameResources::OnBeginFrame(VulkanDevice& device, uint32_t frameIndex)
    {
        m_Descriptors.OnBeginFrame(device, frameIndex);
    }

    void VulkanFrameResources::OnSwapchainRecreated(size_t swapchainImageCount)
    {
        m_Sync.OnSwapchainRecreated(swapchainImageCount);
    }

    bool VulkanFrameResources::IsValid() const
    {
        return m_Initialized && m_FramesInFlight > 0 && m_Sync.IsValid();
    }

    bool VulkanFrameResources::HasDescriptors() const
    {
        return m_Descriptors.IsValid();
    }

    VkCommandBuffer VulkanFrameResources::GetCommandBuffer(uint32_t frameIndex) const
    {
        return m_Command.GetCommandBuffer(frameIndex);
    }

    VkSemaphore VulkanFrameResources::GetImageAvailableSemaphore(uint32_t frameIndex) const
    {
        return m_Sync.GetImageAvailableSemaphore(frameIndex);
    }

    VkSemaphore VulkanFrameResources::GetRenderFinishedSemaphore(uint32_t frameIndex) const
    {
        return m_Sync.GetRenderFinishedSemaphore(frameIndex);
    }

    VkFence VulkanFrameResources::GetInFlightFence(uint32_t frameIndex) const
    {
        return m_Sync.GetInFlightFence(frameIndex);
    }

    VkDescriptorSetLayout VulkanFrameResources::GetGlobalSetLayout() const
    {
        return m_Descriptors.GetGlobalSetLayout();
    }

    VkDescriptorSetLayout VulkanFrameResources::GetMaterialSetLayout() const
    {
        return m_Descriptors.GetMaterialSetLayout();
    }

    VkDescriptorSet VulkanFrameResources::AllocateGlobalSet(uint32_t frameIndex) const
    {
        return m_Descriptors.AllocateGlobalSet(frameIndex);
    }

    VkDescriptorSet VulkanFrameResources::AllocateMaterialSet(uint32_t frameIndex) const
    {
        return m_Descriptors.AllocateMaterialSet(frameIndex);
    }

    const VulkanDescriptors& VulkanFrameResources::GetDescriptors() const
    {
        return m_Descriptors;
    }

    uint32_t VulkanFrameResources::GetFramesInFlight() const
    {
        return m_FramesInFlight;
    }

    void VulkanFrameResources::WaitForFrameFence(VulkanDevice& device, uint32_t frameIndex, uint64_t timeout) const
    {
        m_Sync.WaitForFrameFence(device, frameIndex, timeout);
    }

    void VulkanFrameResources::WaitForImageFenceIfNeeded(VulkanDevice& device, uint32_t imageIndex, uint64_t timeout) const
    {
        m_Sync.WaitForImageFenceIfNeeded(device, imageIndex, timeout);
    }

    void VulkanFrameResources::MarkImageInFlight(uint32_t imageIndex, VkFence owningFrameFence)
    {
        m_Sync.MarkImageInFlight(imageIndex, owningFrameFence);
    }
}