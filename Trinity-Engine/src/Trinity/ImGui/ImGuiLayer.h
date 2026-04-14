#pragma once

#include "Trinity/Layer/Layer.h"

#include <cstdint>
#include <functional>
#include <memory>

namespace Trinity
{
    class Texture;

    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer() override;

        void OnInitialize() override;
        void OnShutdown() override;
        void OnEvent(Event& e) override;

        void Begin();
        void End();

        void SetMenuBarCallback(std::function<void()> callback);

        uint64_t RegisterTexture(const std::shared_ptr<Texture>& texture);
        void UnregisterTexture(uint64_t textureID);

        static ImGuiLayer& Get();

    private:
        void PushDockspace();

        struct Implementation;
        std::unique_ptr<Implementation> m_Implementation;

        std::function<void()> m_MenuBarCallback;

        static ImGuiLayer* s_Instance;
    };
}
