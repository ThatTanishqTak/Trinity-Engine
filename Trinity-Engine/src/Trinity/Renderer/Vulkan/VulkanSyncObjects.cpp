#include "Trinity/Renderer/Vulkan/VulkanSyncObjects.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

#include <limits>

namespace Trinity
{
    void VulkanSyncObjects::Initialize(VkDevice device, uint32_t framesInFlight, uint32_t imageCount)
    {
        m_Device = device;

        m_ImageAvailableSemaphores.resize(framesInFlight);
        m_RenderFinishedSemaphores.resize(imageCount);
        m_InFlightFences.resize(framesInFlight);

        VkSemaphoreCreateInfo l_SemaphoreInfo{};
        l_SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo l_FenceInfo{};
        l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        l_FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < framesInFlight; i++)
        {
            VulkanUtilities::VKCheck(vkCreateSemaphore(device, &l_SemaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]), "Failed vkCreateSemaphore");
            VulkanUtilities::VKCheck(vkCreateFence(device, &l_FenceInfo, nullptr, &m_InFlightFences[i]), "Failed vkCreateFence");
        }

        for (uint32_t i = 0; i < imageCount; i++)
        {
            VulkanUtilities::VKCheck(vkCreateSemaphore(device, &l_SemaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]), "Failed vkCreateSemaphore");
        }

        TR_CORE_INFO("Vulkan sync objects created ({} frames in flight, {} render semaphores).", framesInFlight, imageCount);
    }

    void VulkanSyncObjects::Shutdown()
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            return;
        }

        for (size_t i = 0; i < m_InFlightFences.size(); i++)
        {
            vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
        }

        for (size_t i = 0; i < m_RenderFinishedSemaphores.size(); i++)
        {
            vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
        }

        m_ImageAvailableSemaphores.clear();
        m_RenderFinishedSemaphores.clear();
        m_InFlightFences.clear();
    }

    void VulkanSyncObjects::WaitForFence(uint32_t frameIndex) const
    {
        VulkanUtilities::VKCheck(vkWaitForFences(m_Device, 1, &m_InFlightFences[frameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max()), "Failed vkWaitForFences");
    }

    void VulkanSyncObjects::ResetFence(uint32_t frameIndex) const
    {
        VulkanUtilities::VKCheck(vkResetFences(m_Device, 1, &m_InFlightFences[frameIndex]), "Failed vkResetFences");
    }
}