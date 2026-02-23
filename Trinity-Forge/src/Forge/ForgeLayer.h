#pragma once

#include "Trinity/Layer/Layer.h"
#include "Trinity/Camera/EditorCamera.h"
#include "Trinity/ECS/Scene.h"

#include <imgui.h>
#include <memory>

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
    std::unique_ptr<Trinity::Scene> m_ActiveScene;

    Trinity::EditorCamera m_EditorCamera;
    bool m_CanControlCamera = false;
    bool m_IsLooking = false;
    ImVec2 m_SceneViewportSize = ImVec2(0.0f, 0.0f);
};