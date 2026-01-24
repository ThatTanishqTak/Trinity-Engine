#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Engine
{
    class VulkanDevice;

    class VulkanCommand
    {
    public:
        VulkanCommand() = default;
        ~VulkanCommand() = default;

        VulkanCommand(const VulkanCommand&) = delete;
        VulkanCommand& operator=(const VulkanCommand&) = delete;
        VulkanCommand(VulkanCommand&&) = delete;
        VulkanCommand& operator=(VulkanCommand&&) = delete;

        void Initialize(VulkanDevice& device, uint32_t framesInFlight);
        void Shutdown();

        VkCommandPool GetCommandPool() const { return m_CommandPool; }
        uint32_t GetFramesInFlight() const { return m_FramesInFlight; }

        VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const;

        void ResetCommandBuffer(uint32_t frameIndex);
        void ResetPool(VkCommandPoolResetFlags flags = 0);

        void Begin(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags = 0) const;
        void End(VkCommandBuffer commandBuffer) const;

        // Useful later for staging uploads
        VkCommandBuffer BeginSingleTime() const;
        void EndSingleTime(VkCommandBuffer commandBuffer, VkQueue queue) const;

    private:
        void CreateCommandPool();
        void AllocateCommandBuffers();

    private:
        VulkanDevice* m_Device = nullptr;

        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_CommandBuffers;

        uint32_t m_FramesInFlight = 0;
    };
}