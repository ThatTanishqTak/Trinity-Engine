#include "Engine/Renderer/Vulkan/VulkanSync.h"

#include "Engine/Renderer/Vulkan/VulkanDevice.h"
#include "Engine/Utilities/Utilities.h"

namespace Engine
{
    void VulkanSync::Initialize(VulkanDevice& device, uint32_t maxFramesInFlight, size_t swapchainImageCount)
    {
        Shutdown(device);

        m_MaxFramesInFlight = maxFramesInFlight;

        m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight);
        m_RenderFinishedSemaphores.resize(m_MaxFramesInFlight);
        m_InFlightFences.resize(m_MaxFramesInFlight);

        VkSemaphoreCreateInfo l_SemaphoreCreateInfo{};
        l_SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo l_FenceCreateInfo{};
        l_FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        l_FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < m_MaxFramesInFlight; ++i)
        {
            Utilities::VulkanUtilities::VKCheckStrict(vkCreateSemaphore(device.GetDevice(), &l_SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i]), "vkCreateSemaphore (image)");
            Utilities::VulkanUtilities::VKCheckStrict(vkCreateSemaphore(device.GetDevice(), &l_SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]), "vkCreateSemaphore (render)");
            Utilities::VulkanUtilities::VKCheckStrict(vkCreateFence(device.GetDevice(), &l_FenceCreateInfo, nullptr, &m_InFlightFences[i]), "vkCreateFence");
        }

        m_ImagesInFlight.assign(swapchainImageCount, VK_NULL_HANDLE);

        m_Initialized = true;
    }

    void VulkanSync::Shutdown(VulkanDevice& device)
    {
        if (!m_Initialized)
        {
            m_ImageAvailableSemaphores.clear();
            m_RenderFinishedSemaphores.clear();
            m_InFlightFences.clear();
            m_ImagesInFlight.clear();
            m_MaxFramesInFlight = 0;

            return;
        }

        if (device.GetDevice())
        {
            for (size_t i = 0; i < m_ImageAvailableSemaphores.size(); ++i)
            {
                if (m_ImageAvailableSemaphores[i])
                {
                    vkDestroySemaphore(device.GetDevice(), m_ImageAvailableSemaphores[i], nullptr);
                }

                if (i < m_RenderFinishedSemaphores.size() && m_RenderFinishedSemaphores[i])
                {
                    vkDestroySemaphore(device.GetDevice(), m_RenderFinishedSemaphores[i], nullptr);
                }

                if (i < m_InFlightFences.size() && m_InFlightFences[i])
                {
                    vkDestroyFence(device.GetDevice(), m_InFlightFences[i], nullptr);
                }
            }
        }

        m_ImageAvailableSemaphores.clear();
        m_RenderFinishedSemaphores.clear();
        m_InFlightFences.clear();
        m_ImagesInFlight.clear();

        m_MaxFramesInFlight = 0;
        m_Initialized = false;
    }

    void VulkanSync::OnSwapchainRecreated(size_t swapchainImageCount)
    {
        m_ImagesInFlight.assign(swapchainImageCount, VK_NULL_HANDLE);
    }

    VkSemaphore VulkanSync::GetImageAvailableSemaphore(uint32_t frameIndex) const
    {
        return m_ImageAvailableSemaphores[frameIndex];
    }

    VkSemaphore VulkanSync::GetRenderFinishedSemaphore(uint32_t frameIndex) const
    {
        return m_RenderFinishedSemaphores[frameIndex];
    }

    VkFence VulkanSync::GetInFlightFence(uint32_t frameIndex) const
    {
        return m_InFlightFences[frameIndex];
    }

    void VulkanSync::WaitForFrameFence(VulkanDevice& device, uint32_t frameIndex, uint64_t timeout) const
    {
        Utilities::VulkanUtilities::VKCheckStrict(vkWaitForFences(device.GetDevice(), 1, &m_InFlightFences[frameIndex], VK_TRUE, timeout), "vkWaitForFences");
    }

    void VulkanSync::ResetFrameFence(VulkanDevice& device, uint32_t frameIndex) const
    {
        Utilities::VulkanUtilities::VKCheckStrict(vkResetFences(device.GetDevice(), 1, &m_InFlightFences[frameIndex]), "vkResetFences");
    }

    void VulkanSync::WaitForImageFenceIfNeeded(VulkanDevice& device, uint32_t imageIndex, uint64_t timeout) const
    {
        if (imageIndex < m_ImagesInFlight.size() && m_ImagesInFlight[imageIndex] != VK_NULL_HANDLE)
        {
            Utilities::VulkanUtilities::VKCheckStrict(vkWaitForFences(device.GetDevice(), 1, &m_ImagesInFlight[imageIndex], VK_TRUE, timeout), "vkWaitForFences (image)");
        }
    }

    void VulkanSync::MarkImageInFlight(uint32_t imageIndex, VkFence owningFrameFence)
    {
        if (imageIndex < m_ImagesInFlight.size())
        {
            m_ImagesInFlight[imageIndex] = owningFrameFence;
        }
    }

    bool VulkanSync::IsValid() const
    {
        return m_Initialized && m_MaxFramesInFlight > 0;
    }
}