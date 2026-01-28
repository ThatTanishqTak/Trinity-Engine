#include "Engine/Renderer/Vulkan/VulkanCommand.h"

#include "Engine/Utilities/Utilities.h"

namespace Engine
{
    void VulkanCommand::Initialize(VkDevice device, uint32_t graphicsQueueFamilyIndex)
    {
        m_Device = device;
        m_GraphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
    }

    void VulkanCommand::Shutdown()
    {
        Cleanup();

        m_Device = VK_NULL_HANDLE;
        m_GraphicsQueueFamilyIndex = 0;
    }

    void VulkanCommand::Create(uint32_t maxFramesInFlight)
    {
        CreateCommandPool();
        AllocateCommandBuffers(maxFramesInFlight);
    }

    void VulkanCommand::Cleanup()
    {
        m_CommandBuffers.clear();

        if (m_CommandPool)
        {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }
    }

    void VulkanCommand::CreateCommandPool()
    {
        VkCommandPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        l_PoolInfo.queueFamilyIndex = m_GraphicsQueueFamilyIndex;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateCommandPool(m_Device, &l_PoolInfo, nullptr, &m_CommandPool), "vkCreateCommandPool");
    }

    void VulkanCommand::AllocateCommandBuffers(uint32_t maxFramesInFlight)
    {
        m_CommandBuffers.resize(maxFramesInFlight, VK_NULL_HANDLE);

        VkCommandBufferAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocInfo.commandPool = m_CommandPool;
        l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocInfo.commandBufferCount = maxFramesInFlight;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkAllocateCommandBuffers(m_Device, &l_AllocInfo, m_CommandBuffers.data()), "vkAllocateCommandBuffers");
    }

    VkCommandBuffer VulkanCommand::GetCommandBuffer(uint32_t frameIndex) const
    {
        return m_CommandBuffers.at(frameIndex);
    }

    void VulkanCommand::ResetCommandBuffer(uint32_t frameIndex) const
    {
        vkResetCommandBuffer(GetCommandBuffer(frameIndex), 0);
    }
}