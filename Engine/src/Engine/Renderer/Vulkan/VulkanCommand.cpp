#include "Engine/Renderer/Vulkan/VulkanCommand.h"
#include "Engine/Renderer/Vulkan/VulkanDevice.h"

#include "Engine/Utilities/Utilities.h"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace Engine
{
    void VulkanCommand::Initialize(VulkanDevice& device, uint32_t framesInFlight)
    {
        m_Device = &device;
        m_FramesInFlight = framesInFlight;

        CreateCommandPool();
        AllocateCommandBuffers();
    }

    void VulkanCommand::Shutdown()
    {
        if (!m_Device || !m_Device->GetDevice())
        {
            m_Device = nullptr;
            m_FramesInFlight = 0;
            m_CommandBuffers.clear();
            m_CommandPool = VK_NULL_HANDLE;

            return;
        }

        if (!m_CommandBuffers.empty())
        {
            vkFreeCommandBuffers(m_Device->GetDevice(), m_CommandPool, (uint32_t)m_CommandBuffers.size(), m_CommandBuffers.data());
            m_CommandBuffers.clear();
        }

        if (m_CommandPool)
        {
            vkDestroyCommandPool(m_Device->GetDevice(), m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }

        m_Device = nullptr;
        m_FramesInFlight = 0;
    }

    VkCommandBuffer VulkanCommand::GetCommandBuffer(uint32_t frameIndex) const
    {
        if (frameIndex >= m_CommandBuffers.size())
        {
            TR_CORE_CRITICAL("VulkanCommand::GetCommandBuffer out of range (frameIndex = {}, buffers = {})", frameIndex, (uint32_t)m_CommandBuffers.size());

            return VK_NULL_HANDLE;
        }

        return m_CommandBuffers[frameIndex];
    }

    void VulkanCommand::ResetCommandBuffer(uint32_t frameIndex)
    {
        VkCommandBuffer l_CommandBuffer = GetCommandBuffer(frameIndex);
        if (!l_CommandBuffer || !m_Device || !m_Device->GetDevice())
        {
            return;
        }

        Utilities::VulkanUtilities::VKCheckStrict(vkResetCommandBuffer(l_CommandBuffer, 0), "vkResetCommandBuffer");
    }

    void VulkanCommand::ResetPool(VkCommandPoolResetFlags flags)
    {
        if (!m_Device || !m_Device->GetDevice() || !m_CommandPool)
        {
            return;
        }

        Utilities::VulkanUtilities::VKCheckStrict(vkResetCommandPool(m_Device->GetDevice(), m_CommandPool, flags), "vkResetCommandPool");
    }

    void VulkanCommand::Begin(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags) const
    {
        VkCommandBufferBeginInfo l_CommandBufferBeginInfo{};
        l_CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_CommandBufferBeginInfo.flags = usageFlags;

        Utilities::VulkanUtilities::VKCheckStrict(vkBeginCommandBuffer(commandBuffer, &l_CommandBufferBeginInfo), "vkBeginCommandBuffer");
    }

    void VulkanCommand::End(VkCommandBuffer commandBuffer) const
    {
        Utilities::VulkanUtilities::VKCheckStrict(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");
    }

    VkCommandBuffer VulkanCommand::BeginSingleTime() const
    {
        if (!m_Device || !m_Device->GetDevice() || !m_CommandPool)
        {
            return VK_NULL_HANDLE;
        }

        VkCommandBuffer l_CommandBuffer = VK_NULL_HANDLE;

        VkCommandBufferAllocateInfo l_CommandBufferAllocateInfo{};
        l_CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_CommandBufferAllocateInfo.commandPool = m_CommandPool;
        l_CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_CommandBufferAllocateInfo.commandBufferCount = 1;

        Utilities::VulkanUtilities::VKCheckStrict(vkAllocateCommandBuffers(m_Device->GetDevice(), &l_CommandBufferAllocateInfo, &l_CommandBuffer), "vkAllocateCommandBuffers (single)");

        VkCommandBufferBeginInfo l_CommandBufferBeginInfo{};
        l_CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        Utilities::VulkanUtilities::VKCheckStrict(vkBeginCommandBuffer(l_CommandBuffer, &l_CommandBufferBeginInfo), "vkBeginCommandBuffer (single)");

        return l_CommandBuffer;
    }

    void VulkanCommand::EndSingleTime(VkCommandBuffer commandBuffer, VkQueue queue) const
    {
        if (!m_Device || !m_Device->GetDevice() || !m_CommandPool || !commandBuffer)
        {
            return;
        }

        Utilities::VulkanUtilities::VKCheckStrict(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer (single)");

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &commandBuffer;

        Utilities::VulkanUtilities::VKCheckStrict(vkQueueSubmit(queue, 1, &l_SubmitInfo, VK_NULL_HANDLE), "vkQueueSubmit (single)");
        Utilities::VulkanUtilities::VKCheckStrict(vkQueueWaitIdle(queue), "vkQueueWaitIdle (single)");

        vkFreeCommandBuffers(m_Device->GetDevice(), m_CommandPool, 1, &commandBuffer);
    }

    void VulkanCommand::CreateCommandPool()
    {
        if (!m_Device || !m_Device->GetDevice())
        {
            std::abort;
        }

        VkCommandPoolCreateInfo l_CommandPoolCreateInfo{};
        l_CommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        l_CommandPoolCreateInfo.queueFamilyIndex = m_Device->GetGraphicsQueueFamily();

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateCommandPool(m_Device->GetDevice(), &l_CommandPoolCreateInfo, nullptr, &m_CommandPool), "vkCreateCommandPool");
    }

    void VulkanCommand::AllocateCommandBuffers()
    {
        if (!m_Device || !m_Device->GetDevice() || !m_CommandPool || m_FramesInFlight == 0)
        {
            std::abort;
        }

        m_CommandBuffers.resize(m_FramesInFlight);

        VkCommandBufferAllocateInfo l_CommandBufferAllocateInfo{};
        l_CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_CommandBufferAllocateInfo.commandPool = m_CommandPool;
        l_CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_CommandBufferAllocateInfo.commandBufferCount = m_FramesInFlight;

        Utilities::VulkanUtilities::VKCheckStrict(vkAllocateCommandBuffers(m_Device->GetDevice(), &l_CommandBufferAllocateInfo, m_CommandBuffers.data()), "vkAllocateCommandBuffers");
    }
}