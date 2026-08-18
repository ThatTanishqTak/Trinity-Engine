#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include <imgui.h>
#include <ImGuizmo.h>

#include <glm/glm.hpp>

#include <Forge/Editor/EditorPanel.h>

#include <Trinity/Scene/Components/TransformComponent.h>

namespace Trinity
{
    class Scene;

    class ViewportPanel : public EditorPanel
    {
    public:
        ViewportPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;

    private:
        // Unity's Scene view tool handles, in Q / W / E / R / Y order.
        enum class SceneTool
        {
            View,
            Move,
            Rotate,
            Scale,
            Transform
        };

        void RenderGizmo(const ImVec2& imageMin, const ImVec2& imageSize);
        void RenderToolsOverlay(const ImVec2& viewportMin);
        void RenderOrientationOverlay(const ImVec2& viewportMin);
        void RenderStatsOverlay(const ImVec2& viewportMin, const ImVec2& viewportSize);
        void FocusOnSelection();
        ImGuizmo::OPERATION CurrentOperation() const;
        glm::vec3 SelectionCenter(Scene& scene) const;

    private:
        bool m_Focused = false;
        bool m_Hovered = false;
        bool m_ShowStats = false;
        SceneTool m_Tool = SceneTool::Move;
        // Unity calls these Pivot / Center and Local / Global.
        bool m_PivotCenter = false;
        ImGuizmo::MODE m_GizmoMode = ImGuizmo::LOCAL;
        bool m_GizmoWasUsing = false;

        // Gizmo-drag undo baselines for every top-most selected entity being written.
        std::vector<std::pair<uint64_t, TransformComponent>> m_GizmoStartTransforms;
        bool m_SnapEnabled = false;
        float m_SnapTranslation = 0.5f;
        float m_SnapRotation = 15.0f;
        float m_SnapScale = 0.25f;
    };
}