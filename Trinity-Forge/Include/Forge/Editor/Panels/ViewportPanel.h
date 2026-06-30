#pragma once

#include <imgui.h>
#include <ImGuizmo.h>

#include <Forge/Editor/EditorPanel.h>

#include <Trinity/Scene/Components/TransformComponent.h>

namespace Trinity
{
    class ViewportPanel : public EditorPanel
    {
    public:
        ViewportPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;

    private:
        void RenderGizmo(const ImVec2& imageMin, const ImVec2& imageSize);
        void RenderOverlayToolbar(const ImVec2& viewportMin);

        bool m_Focused = false;
        bool m_Hovered = false;
        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE m_GizmoMode = ImGuizmo::LOCAL;
        bool m_GizmoWasUsing = false;
        TransformComponent m_GizmoStartTransform;
        bool m_SnapEnabled = false;
        float m_SnapTranslation = 0.5f;
        float m_SnapRotation = 15.0f;
        float m_SnapScale = 0.25f;
    };
}