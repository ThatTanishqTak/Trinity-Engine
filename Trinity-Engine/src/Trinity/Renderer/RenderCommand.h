#pragma once

#include "Trinity/Renderer/Renderer.h"

#include <cstdint>
#include <string>

namespace Trinity
{
    class Window;
    class ImGuiLayer;
    class VulkanRenderGraph;

    class RenderCommand
    {
    public:
        static void Initialize(Window& window, RendererAPI api);
        static void Shutdown();

        static void Resize(uint32_t width, uint32_t height);

        static void BeginFrame();
        static void EndFrame();

        static void RenderImGui(ImGuiLayer& imGuiLayer);
        static void SetSceneViewportSize(uint32_t width, uint32_t height);
        static void* GetSceneViewportHandle();

        static Renderer& GetRenderer();
        static VulkanRenderGraph& GetRenderGraph();

    private:
        static std::string ApiToString(RendererAPI api);
    };
}