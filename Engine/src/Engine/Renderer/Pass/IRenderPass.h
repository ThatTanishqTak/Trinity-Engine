#pragma once

#include <cstdint>
#include <span>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace Engine
{
    class VulkanDevice;
    class VulkanFrameResources;
    class VulkanSwapchain;

    struct RenderCube
    {
        glm::vec3 m_Size{ 1.0f };
        glm::vec3 m_Position{ 0.0f };
        glm::vec4 m_Tint{ 1.0f };
    };

    class IRenderPass
    {
    public:
        virtual ~IRenderPass() = default;

        virtual void Initialize(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources) = 0;
        virtual void OnResize(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources) = 0;
        virtual void RecordCommandBuffer(VkCommandBuffer command, uint32_t imageIndex, uint32_t currentFrame, const glm::vec4& clearColor, std::span<const RenderCube> pendingCubes) = 0;
        virtual void OnDestroy(VulkanDevice& device) = 0;
    };
}