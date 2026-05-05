#pragma once

#include "Trinity/Renderer/Vulkan/VulkanCommandList.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
    class VulkanDevice;

    class VulkanCommandPool
    {
    public:
        VulkanCommandPool() = default;
        ~VulkanCommandPool() = default;

        void Initialize(VulkanDevice& device, uint32_t framesInFlight);
        void Shutdown();

        VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const { return m_CommandBuffers[frameIndex]; }
        VulkanCommandList& GetCommandList(uint32_t frameIndex) { return m_CommandLists[frameIndex]; }
        VkCommandPool GetPool() const { return m_CommandPool; }

        void ResetCommandBuffer(uint32_t frameIndex);
        void BeginCommandBuffer(uint32_t frameIndex);
        void EndCommandBuffer(uint32_t frameIndex);

        VkCommandBuffer BeginSingleTimeCommands() const;
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    private:
        VulkanDevice* m_Device = nullptr;
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VulkanCommandList> m_CommandLists;
    };
}