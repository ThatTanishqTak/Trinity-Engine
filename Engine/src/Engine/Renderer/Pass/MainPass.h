#pragma once

#include "Engine/Renderer/Pass/IRenderPass.h"
#include "Engine/Renderer/Vulkan/VulkanPipeline.h"
#include "Engine/Renderer/Vulkan/VulkanResources.h"

#include <vector>

namespace Engine
{
    class MainPass final : public IRenderPass
    {
    public:
        MainPass() = default;
        ~MainPass() override = default;

        MainPass(const MainPass&) = delete;
        MainPass& operator=(const MainPass&) = delete;
        MainPass(MainPass&&) = delete;
        MainPass& operator=(MainPass&&) = delete;

        const char* GetName() const override { return "MainPass"; }
        void Initialize(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources) override;
        void OnResize(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources, VulkanRenderer& renderer) override;
        void RecordCommandBuffer(VkCommandBuffer command, uint32_t imageIndex, uint32_t currentFrame, VulkanRenderer& renderer) override;
        void OnDestroy(VulkanDevice& device) override;

        void SetClearColor(const glm::vec4& clearColor);
        void Clear();
        void DrawCube(const glm::vec3& size, const glm::vec3& position, const glm::vec4& tint);

    private:
        void CreateRenderPass(VulkanDevice& device, VulkanSwapchain& swapchain);
        void CreateFramebuffers(VulkanDevice& device, VulkanSwapchain& swapchain);
        void CreatePipeline(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources);
        void CreateGlobalUniformBuffers(VulkanDevice& device, VulkanFrameResources& frameResources);
        
        void DestroyGlobalUniformBuffers(VulkanDevice& device);
        void DestroySwapchainResources(VulkanDevice& device);
        void DestroySwapchainResources(VulkanDevice& device, VulkanRenderer& renderer);

    private:
        struct GlobalUniformData
        {
            glm::mat4 ViewProjection{ 1.0f };
        };

        // Swapchain-dependent resources.
        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_Framebuffers;
        VulkanPipeline m_Pipeline;
        VkExtent2D m_Extent{};

        VulkanFrameResources* m_FrameResources = nullptr;
        VulkanDevice* m_Device = nullptr;
        std::vector<VulkanResources::BufferResource> m_GlobalUniformBuffers;

        glm::vec4 m_ClearColor{ 0.05f, 0.05f, 0.05f, 1.0f };
        bool m_ClearRequested = true;
        std::vector<RenderCube> m_PendingCubes;
    };
}