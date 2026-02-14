#pragma once

#include "Trinity/Layer/Layer.h"
#include "Trinity/Camera/EditorCamera.h"

#include <imgui.h>

namespace Trinity
{
    class Event;
}

class ForgeLayer : public Trinity::Layer
{
public:
    ForgeLayer();
    ~ForgeLayer() override;

    void OnInitialize() override;
    void OnShutdown() override;

    void OnUpdate(float deltaTime) override;
    void OnRender() override;

    void OnImGuiRender() override;

    void OnEvent(Trinity::Event& e) override;

private:
    Trinity::EditorCamera m_EditorCamera;
    bool m_CanControlCamera = false;
    ImVec2 m_SceneViewportSize = ImVec2(0.0f, 0.0f);
};