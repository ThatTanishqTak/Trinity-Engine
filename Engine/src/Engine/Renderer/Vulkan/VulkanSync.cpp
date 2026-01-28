#include "Engine/Renderer/Vulkan/VulkanSync.h"

#include "Engine/Utilities/Utilities.h"

namespace Engine
{
    void VulkanSync::Initialize(VkDevice device, uint32_t maxFramesInFlight)
    {
        m_Device = device;
        m_MaxFramesInFlight = maxFramesInFlight;
    }

    void VulkanSync::Shutdown()
    {
        DestroyPerImage();
        DestroyPerFrame();

        m_Device = VK_NULL_HANDLE;
        m_MaxFramesInFlight = 0;
    }

    void VulkanSync::CreatePerFrame()
    {
        DestroyPerFrame();

        m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight, VK_NULL_HANDLE);
        m_InFlightFences.resize(m_MaxFramesInFlight, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo l_SemInfo{};
        l_SemInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo l_FenceInfo{};
        l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        l_FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < m_MaxFramesInFlight; ++i)
        {
            Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateSemaphore(m_Device, &l_SemInfo, nullptr, &m_ImageAvailableSemaphores[i]), "vkCreateSemaphore(image available)");
            Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateFence(m_Device, &l_FenceInfo, nullptr, &m_InFlightFences[i]), "vkCreateFence(in flight)");
        }
    }

    void VulkanSync::DestroyPerFrame()
    {
        for (VkSemaphore it_Sem : m_ImageAvailableSemaphores)
        {
            if (it_Sem)
            {
                vkDestroySemaphore(m_Device, it_Sem, nullptr);
            }
        }

        for (VkFence it_Fence : m_InFlightFences)
        {
            if (it_Fence)
            {
                vkDestroyFence(m_Device, it_Fence, nullptr);
            }
        }

        m_ImageAvailableSemaphores.clear();
        m_InFlightFences.clear();
    }

    void VulkanSync::RecreatePerImage(uint32_t swapchainImageCount)
    {
        DestroyPerImage();

        m_RenderFinishedSemaphores.resize(swapchainImageCount, VK_NULL_HANDLE);
        m_ImagesInFlight.resize(swapchainImageCount, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo l_SemInfo{};
        l_SemInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (VkSemaphore& it_Sem : m_RenderFinishedSemaphores)
        {
            Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateSemaphore(m_Device, &l_SemInfo, nullptr, &it_Sem), "vkCreateSemaphore(render finished per image)");
        }
    }

    void VulkanSync::DestroyPerImage()
    {
        for (VkSemaphore it_Sem : m_RenderFinishedSemaphores)
        {
            if (it_Sem)
            {
                vkDestroySemaphore(m_Device, it_Sem, nullptr);
            }
        }

        m_RenderFinishedSemaphores.clear();
        m_ImagesInFlight.clear();
    }

    VkSemaphore VulkanSync::GetImageAvailableSemaphore(uint32_t frameIndex) const
    {
        return m_ImageAvailableSemaphores.at(frameIndex);
    }

    VkSemaphore VulkanSync::GetRenderFinishedSemaphore(uint32_t imageIndex) const
    {
        return m_RenderFinishedSemaphores.at(imageIndex);
    }

    VkFence VulkanSync::GetInFlightFence(uint32_t frameIndex) const
    {
        return m_InFlightFences.at(frameIndex);
    }

    void VulkanSync::WaitForFrame(uint32_t frameIndex) const
    {
        VkFence l_Fence = GetInFlightFence(frameIndex);
        vkWaitForFences(m_Device, 1, &l_Fence, VK_TRUE, UINT64_MAX);
    }

    void VulkanSync::ResetFrame(uint32_t frameIndex) const
    {
        VkFence l_Fence = GetInFlightFence(frameIndex);
        vkResetFences(m_Device, 1, &l_Fence);
    }

    void VulkanSync::WaitForImage(uint32_t imageIndex) const
    {
        if (imageIndex >= m_ImagesInFlight.size())
        {
            return;
        }

        VkFence l_Fence = m_ImagesInFlight[imageIndex];
        if (l_Fence != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_Device, 1, &l_Fence, VK_TRUE, UINT64_MAX);
        }
    }

    void VulkanSync::SetImageInFlight(uint32_t imageIndex, VkFence fence)
    {
        if (imageIndex >= m_ImagesInFlight.size())
        {
            return;
        }

        m_ImagesInFlight[imageIndex] = fence;
    }
}