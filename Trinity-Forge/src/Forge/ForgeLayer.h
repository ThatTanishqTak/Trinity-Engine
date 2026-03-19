#pragma once

#include "Trinity/Layer/Layer.h"
#include "Trinity/Camera/EditorCamera.h"
#include "Trinity/ECS/Scene.h"
#include "Trinity/ECS/Entity.h"

#include "Forge/Panels/ContentBrowserPanel.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <functional>
#include <memory>
#include <string>

namespace Trinity
{
    class Event;
}

enum class EditorState : uint8_t
{
    Edit = 0,
    Play,
    Paused
};

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
    void DrawToolbar();
    void DrawSceneViewport();
    void DrawSceneHierarchy();
    void DrawInspector();

    void DrawEntityNode(Trinity::Entity entity);
    void DrawComponents(Trinity::Entity entity);
    void DrawAddComponentMenu(Trinity::Entity entity);

    template<typename T>
    void DrawComponent(const char* name, Trinity::Entity entity, std::function<void(T&)> uiFunc, bool removable = true);

    void NewScene();
    void OpenScene(const std::string& filePath);
    void SaveScene();
    void SaveSceneAs();

    void OnPlay();
    void OnStop();
    void OnPause();

private:
    std::unique_ptr<Trinity::Scene> m_ActiveScene;
    Trinity::Entity m_SelectedEntity;

    Trinity::EditorCamera m_EditorCamera;
    bool m_CanControlCamera = false;
    bool m_IsLooking = false;
    ImVec2 m_SceneViewportSize = ImVec2(0.0f, 0.0f);
    ImVec2 m_SceneViewportBounds[2]{};

    std::string m_CurrentScenePath;

    EditorState m_EditorState = EditorState::Edit;
    std::string m_SceneSnapshot;

    int m_GizmoOperation = (int)ImGuizmo::TRANSLATE;
    bool m_GizmoLocal = true;
    bool m_GizmoSnap = false;
    float m_TranslationSnap = 0.5f;
    float m_RotationSnap = 45.0f;
    float m_ScaleSnap = 0.5f;

    ContentBrowserPanel m_ContentBrowser;
};

template<typename T>
void ForgeLayer::DrawComponent(const char* name, Trinity::Entity entity, std::function<void(T&)> uiFunc, bool removable)
{
    if (!entity.HasComponent<T>())
    {
        return;
    }

    constexpr ImGuiTreeNodeFlags l_TreeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;

    T& a_Component = entity.GetComponent<T>();
    const ImVec2 l_ContentRegionAvailable = ImGui::GetContentRegionAvail();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
    const float l_LineHeight = ImGui::GetFrameHeight();
    ImGui::Separator();

    const bool l_Open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), l_TreeNodeFlags, "%s", name);
    ImGui::PopStyleVar();

    bool l_RemoveComponent = false;

    if (removable)
    {
        ImGui::SameLine(l_ContentRegionAvailable.x - l_LineHeight * 0.5f);

        ImGui::PushID((int)typeid(T).hash_code());
        if (ImGui::Button("...", ImVec2(l_LineHeight, l_LineHeight)))
        {
            ImGui::OpenPopup("##ComponentOptions");
        }

        if (ImGui::BeginPopup("##ComponentOptions"))
        {
            if (ImGui::MenuItem("Remove Component"))
            {
                l_RemoveComponent = true;
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    if (l_Open)
    {
        uiFunc(a_Component);
        ImGui::TreePop();
    }

    if (l_RemoveComponent)
    {
        entity.RemoveComponent<T>();
    }
}