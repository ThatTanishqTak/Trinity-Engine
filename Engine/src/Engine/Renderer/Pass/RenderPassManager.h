#pragma once

#include "Engine/Renderer/Pass/IRenderPass.h"

#include <cstdint>
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

namespace Engine
{
    class VulkanDevice;
    class VulkanFrameResources;
    class VulkanRenderer;
    class VulkanSwapchain;

    class RenderPassManager
    {
    public:
        void AddPass(std::unique_ptr<IRenderPass> pass);
        bool RemovePass(const char* passName);

        void OnCreateAll(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources);
        void OnDestroyAll(VulkanDevice& device);
        void OnResizeAll(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources, VulkanRenderer& renderer);
        void RecordAll(VkCommandBuffer command, uint32_t imageIndex, uint32_t currentFrame, const glm::vec4& clearColor, std::span<const RenderCube> pendingCubes, 
            VulkanRenderer& renderer);

    private:
        std::vector<std::unique_ptr<IRenderPass>> m_Passes;
        bool m_IsCreated = false;
    };
}