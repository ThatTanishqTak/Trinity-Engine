#pragma once

#include <functional>

#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanCommands
    {
    public:
        VulkanCommands() = default;
        ~VulkanCommands();

        VulkanCommands(const VulkanCommands&) = delete;
        VulkanCommands& operator=(const VulkanCommands&) = delete;

        bool Initialize(VkDevice device, uint32_t graphicsQueueFamily, VkQueue graphicsQueue);
        void Shutdown();

        VkCommandPool GetPool() const { return m_Pool; }

        void ImmediateSubmit(const std::function<void(VkCommandBuffer)>& recorder);

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkCommandPool m_Pool = VK_NULL_HANDLE;
        VkFence m_ImmediateFence = VK_NULL_HANDLE;
    };
}