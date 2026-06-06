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

        bool m_Focused = false;
        bool m_Hovered = false;
        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
        bool m_GizmoWasUsing = false;
        TransformComponent m_GizmoStartTransform;
    };
}