#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace Engine
{
    class VulkanCommand
    {
    public:
        void Initialize(VkDevice device, uint32_t graphicsQueueFamilyIndex);
        void Shutdown();

        void Create(uint32_t maxFramesInFlight);
        void Cleanup();

        VkCommandPool GetCommandPool() const { return m_CommandPool; }

        VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const;
        uint32_t GetCommandBufferCount() const { return static_cast<uint32_t>(m_CommandBuffers.size()); }

        void ResetCommandBuffer(uint32_t frameIndex) const;

    private:
        void CreateCommandPool();
        void AllocateCommandBuffers(uint32_t maxFramesInFlight);

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        uint32_t m_GraphicsQueueFamilyIndex = 0;

        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_CommandBuffers;
    };
}