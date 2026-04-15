#pragma once

#include "Forge/Panels/Panel.h"

#include "Trinity/Renderer/Camera/EditorCamera.h"
#include "Trinity/Renderer/SceneRenderer.h"

#include <imgui.h>
#include <ImGuizmo.h>

#include "Forge/SelectionContext.h"

#include <cstdint>

namespace Forge
{
    class ViewportPanel : public Panel
    {
    public:
        ViewportPanel(std::string name, SelectionContext* context);

        void OnInitialize() override;
        void OnShutdown() override;
        void OnUpdate(float deltaTime) override;
        void OnRender() override;

    private:
        void RenderGizmos();

        Trinity::SceneRenderer m_SceneRenderer;
        Trinity::EditorCamera m_Camera;

        SelectionContext* m_SelectionContext = nullptr;

        ImGuizmo::OPERATION m_GizmoOperation = static_cast<ImGuizmo::OPERATION>(-1);

        uint64_t m_ViewportTextureID = 0;
        uint32_t m_LastWidth = 0;
        uint32_t m_LastHeight = 0;

        float m_CameraSpeed = 5.0f;
        float m_Sensitivity = 0.1f;

        bool m_IsViewportHovered = false;
        bool m_IsViewportFocused = false;
    };
}
