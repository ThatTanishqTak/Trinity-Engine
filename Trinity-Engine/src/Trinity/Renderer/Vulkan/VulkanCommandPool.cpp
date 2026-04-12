#include "Trinity/Renderer/Vulkan/VulkanCommandPool.h"

#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

namespace Trinity
{
    void VulkanCommandPool::Initialize(VulkanDevice& device, uint32_t framesInFlight)
    {
        m_Device = &device;

        VkCommandPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        l_PoolInfo.queueFamilyIndex = device.GetQueueFamilyIndices().GraphicsFamily.value();

        VulkanUtilities::VKCheck(vkCreateCommandPool(device.GetDevice(), &l_PoolInfo, nullptr, &m_CommandPool), "Failed vkCreateCommandPool");

        m_CommandBuffers.resize(framesInFlight);

        VkCommandBufferAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocInfo.commandPool = m_CommandPool;
        l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocInfo.commandBufferCount = framesInFlight;

        VulkanUtilities::VKCheck(vkAllocateCommandBuffers(device.GetDevice(), &l_AllocInfo, m_CommandBuffers.data()), "Failed vkAllocateCommandBuffers");

        TR_CORE_INFO("Vulkan command pool created ({} command buffers).", framesInFlight);
    }

    void VulkanCommandPool::Shutdown()
    {
        if (m_CommandPool != VK_NULL_HANDLE && m_Device != nullptr)
        {
            vkDestroyCommandPool(m_Device->GetDevice(), m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }

        m_CommandBuffers.clear();
    }

    void VulkanCommandPool::ResetCommandBuffer(uint32_t frameIndex) const
    {
        VulkanUtilities::VKCheck(vkResetCommandBuffer(m_CommandBuffers[frameIndex], 0), "Failed vkResetCommandBuffer");
    }

    void VulkanCommandPool::BeginCommandBuffer(uint32_t frameIndex) const
    {
        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VulkanUtilities::VKCheck(vkBeginCommandBuffer(m_CommandBuffers[frameIndex], &l_BeginInfo), "Failed vkBeginCommandBuffer");
    }

    void VulkanCommandPool::EndCommandBuffer(uint32_t frameIndex) const
    {
        VulkanUtilities::VKCheck(vkEndCommandBuffer(m_CommandBuffers[frameIndex]), "Failed vkEndCommandBuffer");
    }

    VkCommandBuffer VulkanCommandPool::BeginSingleTimeCommands() const
    {
        VkCommandBufferAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocInfo.commandPool = m_CommandPool;
        l_AllocInfo.commandBufferCount = 1;

        VkCommandBuffer l_CommandBuffer;
        VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_Device->GetDevice(), &l_AllocInfo, &l_CommandBuffer), "Failed vkAllocateCommandBuffers");

        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VulkanUtilities::VKCheck(vkBeginCommandBuffer(l_CommandBuffer, &l_BeginInfo), "Failed vkBeginCommandBuffer");

        return l_CommandBuffer;
    }

    void VulkanCommandPool::EndSingleTimeCommands(VkCommandBuffer commandBuffer) const
    {
        VulkanUtilities::VKCheck(vkEndCommandBuffer(commandBuffer), "Failed vkEndCommandBuffer");

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &commandBuffer;

        VulkanUtilities::VKCheck(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &l_SubmitInfo, VK_NULL_HANDLE), "Failed vkQueueSubmit");
        vkQueueWaitIdle(m_Device->GetGraphicsQueue());

        vkFreeCommandBuffers(m_Device->GetDevice(), m_CommandPool, 1, &commandBuffer);
    }
}