#include "Trinity/Renderer/Vulkan/VulkanCommandPool.h"

#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

#include <limits>

namespace Trinity
{
    void VulkanCommandPool::Initialize(VulkanDevice& device, uint32_t framesInFlight)
    {
        TR_CORE_TRACE("Initializing Command Pool");

        m_Device = &device;

        VkCommandPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        l_PoolInfo.queueFamilyIndex = device.GetQueueFamilyIndices().GraphicsFamily.value();

        VulkanUtilities::VKCheck(vkCreateCommandPool(device.GetDevice(), &l_PoolInfo, nullptr, &m_CommandPool), "Failed vkCreateCommandPool");

        TR_CORE_TRACE("Command Pool Initialized");

        TR_CORE_TRACE("Initializing Command Lists");

        m_CommandBuffers.resize(framesInFlight);

        VkCommandBufferAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocInfo.commandPool = m_CommandPool;
        l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocInfo.commandBufferCount = framesInFlight;

        VulkanUtilities::VKCheck(vkAllocateCommandBuffers(device.GetDevice(), &l_AllocInfo, m_CommandBuffers.data()), "Failed vkAllocateCommandBuffers");

        m_CommandLists.reserve(framesInFlight);
        for (uint32_t i = 0; i < framesInFlight; i++)
        {
            m_CommandLists.emplace_back();
            m_CommandLists.back().Initialize(device.GetDevice());
        }

        TR_CORE_TRACE("Command Lists Initialized");
    }

    void VulkanCommandPool::Shutdown()
    {
        TR_CORE_TRACE("Shutting Down Command Lists");

        for (auto& it_List : m_CommandLists)
        {
            it_List.Shutdown();
        }
        m_CommandLists.clear();

        TR_CORE_TRACE("Command Lists Shutdown Complete");

        TR_CORE_TRACE("Shutting Down Command Pool");

        if (m_CommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_Device->GetDevice(), m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }
        m_CommandBuffers.clear();

        TR_CORE_TRACE("Command Pool Shutdown Complete");
    }

    void VulkanCommandPool::ResetCommandBuffer(uint32_t frameIndex)
    {
        VulkanUtilities::VKCheck(vkResetCommandBuffer(m_CommandBuffers[frameIndex], 0), "Failed vkResetCommandBuffer");
        m_CommandLists[frameIndex].Invalidate();
    }

    void VulkanCommandPool::BeginCommandBuffer(uint32_t frameIndex)
    {
        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VulkanUtilities::VKCheck(vkBeginCommandBuffer(m_CommandBuffers[frameIndex], &l_BeginInfo), "Failed vkBeginCommandBuffer");

        m_CommandLists[frameIndex].Reset(m_CommandBuffers[frameIndex]);
    }

    void VulkanCommandPool::EndCommandBuffer(uint32_t frameIndex)
    {
        VulkanUtilities::VKCheck(vkEndCommandBuffer(m_CommandBuffers[frameIndex]), "Failed vkEndCommandBuffer");
        m_CommandLists[frameIndex].Invalidate();
    }

    VkCommandBuffer VulkanCommandPool::BeginSingleTimeCommands() const
    {
        VkCommandBufferAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocateInfo.commandPool = m_CommandPool;
        l_AllocateInfo.commandBufferCount = 1;

        VkCommandBuffer l_CommandBuffer;
        VulkanUtilities::VKCheck(vkAllocateCommandBuffers(m_Device->GetDevice(), &l_AllocateInfo, &l_CommandBuffer), "Failed vkAllocateCommandBuffers");

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

        VkFenceCreateInfo l_FenceInfo{};
        l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkFence l_Fence = VK_NULL_HANDLE;
        VulkanUtilities::VKCheck(vkCreateFence(m_Device->GetDevice(), &l_FenceInfo, nullptr, &l_Fence), "Failed vkCreateFence");

        VulkanUtilities::VKCheck(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &l_SubmitInfo, l_Fence), "Failed vkQueueSubmit");
        VulkanUtilities::VKCheck(vkWaitForFences(m_Device->GetDevice(), 1, &l_Fence, VK_TRUE, std::numeric_limits<uint64_t>::max()), "Failed vkWaitForFences");

        vkDestroyFence(m_Device->GetDevice(), l_Fence, nullptr);
        vkFreeCommandBuffers(m_Device->GetDevice(), m_CommandPool, 1, &commandBuffer);
    }
}