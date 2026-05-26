#pragma once

#include "Trinity/Layer/Layer.h"
#include "Trinity/Scene/Scene.h"
#include "Trinity/Renderer/SceneRenderer.h"
#include "Trinity/Renderer/Camera/EditorCamera.h"

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

    void OnEvent(Trinity::Event& e) override;

private:
    Trinity::Scene m_Scene;
    Trinity::EditorCamera m_Camera;
    Trinity::SceneRenderer m_SceneRenderer;
};