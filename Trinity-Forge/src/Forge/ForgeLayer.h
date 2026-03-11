#pragma once

#include "Trinity/Layer/Layer.h"
#include "Trinity/Camera/EditorCamera.h"
#include "Trinity/ECS/Scene.h"
#include "Trinity/ECS/Entity.h"

#include <imgui.h>
#include <memory>
#include <string>

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
    void LoadScene(std::unique_ptr<Trinity::Scene> scene);

private:
    void DrawMenuBar();

    void NewScene();
    void OpenScene(const std::string& filePath);
    void SaveScene();
    void SaveSceneAs();

private:
    std::unique_ptr<Trinity::Scene> m_ActiveScene;
    Trinity::Entity m_SelectedEntity;

    Trinity::EditorCamera m_EditorCamera;
    bool m_CanControlCamera = false;
    bool m_IsLooking = false;
    ImVec2 m_SceneViewportSize = ImVec2(0.0f, 0.0f);

    std::string m_CurrentScenePath;
};