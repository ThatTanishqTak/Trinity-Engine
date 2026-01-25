#pragma once

#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Pass/RenderPassManager.h"
#include "Engine/Renderer/Vulkan/VulkanContext.h"
#include "Engine/Renderer/Vulkan/VulkanDevice.h"
#include "Engine/Renderer/Vulkan/VulkanSwapchain.h"
#include "Engine/Renderer/Vulkan/VulkanFrameResources.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

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

        void SetClearColor(const glm::vec4& a_Color) override;
        void Clear() override;
        void DrawCube(const glm::vec3& a_Size, const glm::vec3& a_Position, const glm::vec4& a_Tint) override;

        void OnResize(uint32_t width, uint32_t height) override;

    private:
        void RecreateSwapchain();

    private:
        Window* m_Window = nullptr;
        GLFWwindow* m_GLFWWindow = nullptr;

        VulkanContext m_Context;
        VulkanDevice m_Device;
        VulkanSwapchain m_Swapchain;
        VulkanFrameResources m_FrameResources;
        RenderPassManager m_PassManager;

        static constexpr int s_MaxFramesInFlight = 2;
        int m_CurrentFrame = 0;

        bool m_FramebufferResized = false;
        uint32_t m_ImageIndex = 0;

        bool m_FrameInProgress = false;
        bool m_Initialized = false;

        bool m_LastVSync = true;

        glm::vec4 m_ClearColor = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
        bool m_ClearRequested = true;
        std::vector<RenderCube> m_PendingCubes;
    };
}