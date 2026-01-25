#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

namespace Engine
{
    class IRenderPass;
    class VulkanRenderer;

    struct VulkanFrameResources
    {
        struct PerFrame;
    };

    class RenderPassManager
    {
    public:
        void AddPass(std::unique_ptr<IRenderPass> pass);
        bool RemovePass(const char* passName);

        void OnCreateAll(VulkanRenderer& renderer);
        void OnDestroyAll(VulkanRenderer& renderer);
        void OnResizeAll(VulkanRenderer& renderer);
        void RecordAll(VulkanRenderer& renderer, VkCommandBuffer cmd, uint32_t imageIndex, VulkanFrameResources::PerFrame& frame);

    private:
        std::vector<std::unique_ptr<IRenderPass>> m_Passes;
        bool m_IsCreated = false;
    };
}