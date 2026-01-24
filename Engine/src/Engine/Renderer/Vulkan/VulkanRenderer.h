#pragma once

#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Vulkan/VulkanContext.h"
#include "Engine/Renderer/Vulkan/VulkanDevice.h"
#include "Engine/Renderer/Vulkan/VulkanSwapchain.h"
#include "Engine/Renderer/Vulkan/VulkanCommand.h"
#include "Engine/Renderer/Vulkan/VulkanDescriptors.h"
#include "Engine/Renderer/Vulkan/VulkanSync.h"
#include "Engine/Renderer/Vulkan/VulkanPipeline.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

struct GLFWwindow;

namespace Engine
{
    class Window;

    class VulkanRenderer final : public Renderer
    {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer() override;

        VulkanRenderer(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(const VulkanRenderer&) = delete;
        VulkanRenderer(VulkanRenderer&&) = delete;
        VulkanRenderer& operator=(VulkanRenderer&&) = delete;

        void Initialize(Window& window) override;
        void Shutdown() override;

        void BeginFrame() override;
        void EndFrame() override;

        void OnResize(uint32_t width, uint32_t height) override;

    private:
        void CreateRenderPass();
        void CreateFramebuffers();

        void CreatePipeline();
        void CleanupSwapchainDependents();
        void RecreateSwapchain();

        void RecordCommandBuffer(VkCommandBuffer command, uint32_t imageIndex);

    private:
        Window* m_Window = nullptr;
        GLFWwindow* m_GLFWWindow = nullptr;

        VulkanContext m_Context;
        VulkanDevice m_Device;
        VulkanSwapchain m_Swapchain;
        VulkanCommand m_Command;
        VulkanSync m_Sync;
        VulkanDescriptors m_Descriptors;
        VulkanPipeline m_Pipeline;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_Framebuffers;

        static constexpr int s_MaxFramesInFlight = 2;
        int m_CurrentFrame = 0;

        bool m_FramebufferResized = false;
        uint32_t m_ImageIndex = 0;

        bool m_FrameInProgress = false;
        bool m_Initialized = false;

        bool m_LastVSync = true;
    };
}