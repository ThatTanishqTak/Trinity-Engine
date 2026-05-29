#include <Trinity/Renderer/Backends/Vulkan/VulkanCommands.h>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    VulkanCommands::~VulkanCommands()
    {
        Shutdown();
    }

    bool VulkanCommands::Initialize(VkDevice a_Device, uint32_t a_GraphicsQueueFamily, VkQueue a_GraphicsQueue)
    {
        m_Device = a_Device;
        m_GraphicsQueue = a_GraphicsQueue;

        VkCommandPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        l_PoolInfo.queueFamilyIndex = a_GraphicsQueueFamily;

        VkResult l_Result = vkCreateCommandPool(m_Device, &l_PoolInfo, nullptr, &m_Pool);
        if (l_Result != VK_SUCCESS)
        {
            TR_CORE_CRITICAL("VulkanCommands: vkCreateCommandPool failed ({})", static_cast<int>(l_Result));
            return false;
        }

        VkFenceCreateInfo l_FenceInfo{};
        l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        l_Result = vkCreateFence(m_Device, &l_FenceInfo, nullptr, &m_ImmediateFence);
        if (l_Result != VK_SUCCESS)
        {
            TR_CORE_CRITICAL("VulkanCommands: vkCreateFence failed ({})", static_cast<int>(l_Result));
            return false;
        }

        TR_CORE_INFO("VulkanCommands: created");
        return true;
    }

    void VulkanCommands::Shutdown()
    {
        if (m_ImmediateFence != VK_NULL_HANDLE)
        {
            vkDestroyFence(m_Device, m_ImmediateFence, nullptr);
            m_ImmediateFence = VK_NULL_HANDLE;
        }

        if (m_Pool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_Device, m_Pool, nullptr);
            m_Pool = VK_NULL_HANDLE;
        }
    }

    void VulkanCommands::ImmediateSubmit(const std::function<void(VkCommandBuffer)>& a_Recorder)
    {
        VkCommandBufferAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocInfo.commandPool = m_Pool;
        l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocInfo.commandBufferCount = 1;

        VkCommandBuffer l_CommandBuffer = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(m_Device, &l_AllocInfo, &l_CommandBuffer) != VK_SUCCESS)
        {
            TR_CORE_ERROR("VulkanCommands::ImmediateSubmit: vkAllocateCommandBuffers failed");
            return;
        }

        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(l_CommandBuffer, &l_BeginInfo);

        a_Recorder(l_CommandBuffer);

        vkEndCommandBuffer(l_CommandBuffer);

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;

        vkQueueSubmit(m_GraphicsQueue, 1, &l_SubmitInfo, m_ImmediateFence);

        vkWaitForFences(m_Device, 1, &m_ImmediateFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_Device, 1, &m_ImmediateFence);

        vkFreeCommandBuffers(m_Device, m_Pool, 1, &l_CommandBuffer);
    }
}