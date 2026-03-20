#pragma once

#include "Trinity/Renderer/Renderer.h"

namespace Trinity
{
    // WILL BE WORKED ON IN THE FUTURE
    class MoltenVKRenderer : public Renderer
    {
    public:
        MoltenVKRenderer();
        ~MoltenVKRenderer() override;

        void SetWindow(Window& window) override;

        void Initialize() override;
        void Shutdown() override;

        void Resize(uint32_t width, uint32_t height) override;

        void BeginFrame() override;
        void EndFrame() override;
        void RenderImGui(ImGuiLayer& imGuiLayer) override;
        void SetSceneViewportSize(uint32_t width, uint32_t height) override;
        void* GetSceneViewportHandle() const override;
    };
}