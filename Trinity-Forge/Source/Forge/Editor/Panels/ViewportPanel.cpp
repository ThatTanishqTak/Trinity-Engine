#include <Forge/Editor/Panels/ViewportPanel.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>
#include <Forge/Editor/EditorTheme.h>
#include <Forge/Editor/Commands/PropertyCommands.h>
#include <Forge/Editor/Commands/CompositeCommand.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Renderer/Frontend/Camera.h>
#include <Trinity/Renderer/Frontend/EditorCamera.h>
#include <Trinity/Renderer/Frontend/Renderer.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/HierarchyComponent.h>
#include <Trinity/Scene/Components/IDComponent.h>

namespace Trinity
{
    static bool HasSelectedAncestor(Scene& scene, entt::entity entity, const std::vector<entt::entity>& selection)
    {
        entt::registry& l_Registry = scene.GetRegistry();
        const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(entity);
        entt::entity l_Current = l_Hierarchy != nullptr ? l_Hierarchy->Parent : entt::null;
        while (l_Current != entt::null)
        {
            if (std::find(selection.begin(), selection.end(), l_Current) != selection.end())
            {
                return true;
            }

            const HierarchyComponent* l_ParentHierarchy = l_Registry.try_get<HierarchyComponent>(l_Current);
            l_Current = l_ParentHierarchy != nullptr ? l_ParentHierarchy->Parent : entt::null;
        }

        return false;
    }

    // Unity paints Scene-view overlays as floating rounded panels over the render target.
    struct SceneOverlay
    {
        ImDrawList* DrawList = nullptr;
        ImVec2 Padding = ImVec2(5.0f, 4.0f);
    };

    static SceneOverlay BeginSceneOverlay(const ImVec2& position, const ImVec2& padding)
    {
        SceneOverlay l_Overlay;
        l_Overlay.DrawList = ImGui::GetWindowDrawList();
        l_Overlay.Padding = padding;

        // Split so the panel can be drawn behind the controls once they have been measured.
        l_Overlay.DrawList->ChannelsSplit(2);
        l_Overlay.DrawList->ChannelsSetCurrent(1);

        ImGui::SetCursorScreenPos(ImVec2(position.x + padding.x, position.y + padding.y));
        ImGui::BeginGroup();

        return l_Overlay;
    }

    static void EndSceneOverlay(const SceneOverlay& overlay)
    {
        ImGui::EndGroup();

        ImVec2 l_Min = ImGui::GetItemRectMin();
        ImVec2 l_Max = ImGui::GetItemRectMax();
        ImVec2 l_PanelMin = ImVec2(l_Min.x - overlay.Padding.x, l_Min.y - overlay.Padding.y);
        ImVec2 l_PanelMax = ImVec2(l_Max.x + overlay.Padding.x, l_Max.y + overlay.Padding.y);

        overlay.DrawList->ChannelsSetCurrent(0);
        overlay.DrawList->AddRectFilled(l_PanelMin, l_PanelMax, EditorColors::OverlayBackground, 4.0f);
        overlay.DrawList->AddRect(l_PanelMin, l_PanelMax, EditorColors::DefaultBorder, 4.0f);
        overlay.DrawList->ChannelsMerge();
    }

    ImGuizmo::OPERATION ViewportPanel::CurrentOperation() const
    {
        switch (m_Tool)
        {
            case SceneTool::Rotate:
                return ImGuizmo::ROTATE;
            case SceneTool::Scale:
                return ImGuizmo::SCALE;
            case SceneTool::Transform:
                return static_cast<ImGuizmo::OPERATION>(ImGuizmo::TRANSLATE | ImGuizmo::ROTATE | ImGuizmo::SCALE);
            default:
                return ImGuizmo::TRANSLATE;
        }
    }

    glm::vec3 ViewportPanel::SelectionCenter(Scene& scene) const
    {
        glm::vec3 l_Sum(0.0f);
        int l_Count = 0;

        for (entt::entity it_Entity : m_Context.Selection)
        {
            if (!scene.GetRegistry().valid(it_Entity))
            {
                continue;
            }

            l_Sum += glm::vec3(scene.GetWorldMatrix(it_Entity)[3]);
            ++l_Count;
        }

        return l_Count > 0 ? l_Sum / static_cast<float>(l_Count) : glm::vec3(0.0f);
    }

    void ViewportPanel::OnImGuiRender()
    {
        if (!m_Context.ShowScene)
        {
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(ICON_TR_SCENE "  Scene", &m_Context.ShowScene);

        m_Focused = ImGui::IsWindowFocused();
        m_Hovered = ImGui::IsWindowHovered();

        m_Engine.SetViewportInteractive(m_Hovered);

        if (m_Hovered && !ImGui::GetIO().WantTextInput)
        {
            Input& l_Input = m_Engine.GetPlatform().GetInput();
            if (!l_Input.IsMouseButtonPressed(Mouse::ButtonRight))
            {
                // Unity binds Q / W / E / R / Y to View / Move / Rotate / Scale / Transform.
                if (l_Input.IsKeyPressed(Key::Q))
                {
                    m_Tool = SceneTool::View;
                }

                if (l_Input.IsKeyPressed(Key::W))
                {
                    m_Tool = SceneTool::Move;
                }

                if (l_Input.IsKeyPressed(Key::E))
                {
                    m_Tool = SceneTool::Rotate;
                }

                if (l_Input.IsKeyPressed(Key::R))
                {
                    m_Tool = SceneTool::Scale;
                }

                if (l_Input.IsKeyPressed(Key::Y))
                {
                    m_Tool = SceneTool::Transform;
                }

                if (l_Input.IsKeyPressed(Key::F))
                {
                    FocusOnSelection();
                }
            }
            else
            {
                // Flying (RMB held): the mouse wheel scales the camera speed, as in Unity's fly mode.
                float l_Wheel = ImGui::GetIO().MouseWheel;
                if (l_Wheel != 0.0f && m_Engine.HasEditorCamera())
                {
                    EditorCamera& l_Camera = m_Engine.GetEditorCameraController();
                    l_Camera.SetMoveSpeed(l_Camera.GetMoveSpeed() * std::pow(1.15f, l_Wheel));
                }
            }
        }

        ImVec2 l_Available = ImGui::GetContentRegionAvail();
        ImVec2 l_FramebufferScale = ImGui::GetIO().DisplayFramebufferScale;
        uint32_t l_Width = l_Available.x > 0.0f ? static_cast<uint32_t>(l_Available.x * l_FramebufferScale.x) : 0;
        uint32_t l_Height = l_Available.y > 0.0f ? static_cast<uint32_t>(l_Available.y * l_FramebufferScale.y) : 0;

        if (l_Width > 0 && l_Height > 0)
        {
            m_Engine.SetViewportSize(l_Width, l_Height);

            uint64_t l_TextureId = m_Engine.GetViewportTextureID();
            if (l_TextureId != 0)
            {
                ImGui::Image((ImTextureID)l_TextureId, l_Available);
                ImVec2 l_ImageMin = ImGui::GetItemRectMin();
                ImVec2 l_ImageSize = ImGui::GetItemRectSize();
                RenderGizmo(l_ImageMin, l_ImageSize);
                RenderToolsOverlay(l_ImageMin);
                RenderOrientationOverlay(l_ImageMin);
                RenderStatsOverlay(l_ImageMin, l_ImageSize);
            }
            else
            {
                ImGui::TextUnformatted("Initializing viewport...");
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::RenderGizmo(const ImVec2& imageMin, const ImVec2& imageSize)
    {
        // Unity's View tool shows no handle at all; the scene is camera-only until another tool is picked.
        if (m_Tool == SceneTool::View || !m_Engine.HasScene() || m_Context.SelectedEntity == entt::null)
        {
            return;
        }

        Scene& l_Scene = m_Engine.GetScene();
        entt::registry& l_Registry = l_Scene.GetRegistry();
        if (!l_Registry.valid(m_Context.SelectedEntity) || !l_Registry.all_of<TransformComponent>(m_Context.SelectedEntity))
        {
            return;
        }

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);

        glm::mat4 l_View = m_Engine.GetEditorCamera().GetView();
        glm::mat4 l_Projection = m_Engine.GetEditorCamera().GetProjection();
        glm::mat4 l_PrimaryWorld = l_Scene.GetWorldMatrix(m_Context.SelectedEntity);

        bool l_PrimaryIsHandle = !m_PivotCenter && m_GizmoMode == ImGuizmo::LOCAL;
        glm::mat4 l_World = l_PrimaryWorld;
        if (!l_PrimaryIsHandle)
        {
            glm::vec3 l_Origin = m_PivotCenter ? SelectionCenter(l_Scene) : glm::vec3(l_PrimaryWorld[3]);
            glm::mat4 l_Rotation(1.0f);
            if (m_GizmoMode == ImGuizmo::LOCAL)
            {
                l_Rotation = glm::mat4(glm::mat3(glm::normalize(glm::vec3(l_PrimaryWorld[0])), glm::normalize(glm::vec3(l_PrimaryWorld[1])), glm::normalize(glm::vec3(l_PrimaryWorld[2]))));
            }

            l_World = glm::translate(glm::mat4(1.0f), l_Origin) * l_Rotation;
        }

        glm::mat4 l_WorldBefore = l_World;

        float l_SnapValues[3] = { 0.0f, 0.0f, 0.0f };
        float* l_Snap = nullptr;
        if (m_SnapEnabled)
        {
            float l_Amount = m_SnapTranslation;
            if (m_Tool == SceneTool::Rotate)
            {
                l_Amount = m_SnapRotation;
            }
            else if (m_Tool == SceneTool::Scale)
            {
                l_Amount = m_SnapScale;
            }

            l_SnapValues[0] = l_SnapValues[1] = l_SnapValues[2] = l_Amount;
            l_Snap = l_SnapValues;
        }

        ImGuizmo::Manipulate(glm::value_ptr(l_View), glm::value_ptr(l_Projection), CurrentOperation(), m_GizmoMode, glm::value_ptr(l_World), nullptr, l_Snap);

        bool l_IsUsing = ImGuizmo::IsUsing();

        if (l_IsUsing)
        {
            if (!m_GizmoWasUsing)
            {
                // Snapshot the top-most selected transforms as undo baselines; children of selected parents follow through the hierarchy and are not written directly.
                m_GizmoStartTransforms.clear();
                for (entt::entity it_Entity : m_Context.Selection)
                {
                    if (!l_Registry.valid(it_Entity) || !l_Registry.all_of<TransformComponent>(it_Entity))
                    {
                        continue;
                    }

                    if (HasSelectedAncestor(l_Scene, it_Entity, m_Context.Selection))
                    {
                        continue;
                    }

                    uint64_t l_UUID = static_cast<uint64_t>(Entity(it_Entity, &l_Scene).GetUUID());
                    m_GizmoStartTransforms.emplace_back(l_UUID, l_Registry.get<TransformComponent>(it_Entity));
                }
            }

            glm::mat4 l_Delta = l_World * glm::inverse(l_WorldBefore);

            auto l_ApplyWorld = [&](entt::entity entity, const glm::mat4& world)
                {
                    glm::mat4 l_ParentWorld(1.0f);
                    const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(entity);
                    if (l_Hierarchy != nullptr && l_Hierarchy->Parent != entt::null)
                    {
                        l_ParentWorld = l_Scene.GetWorldMatrix(l_Hierarchy->Parent);
                    }

                    glm::mat4 l_Local = glm::inverse(l_ParentWorld) * world;

                    float l_Translation[3];
                    float l_Rotation[3];
                    float l_Scale[3];
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(l_Local), l_Translation, l_Rotation, l_Scale);

                    TransformComponent& l_Transform = l_Registry.get<TransformComponent>(entity);
                    l_Transform.Translation = glm::vec3(l_Translation[0], l_Translation[1], l_Translation[2]);
                    l_Transform.SetEulerAngles(glm::radians(glm::vec3(l_Rotation[0], l_Rotation[1], l_Rotation[2])));
                    l_Transform.Scale = glm::vec3(l_Scale[0], l_Scale[1], l_Scale[2]);
                };

            for (const auto& it_Start : m_GizmoStartTransforms)
            {
                Entity l_Target = FindEntityByUUID(l_Scene, it_Start.first);
                if (!l_Target.IsValid() || !l_Target.HasComponent<TransformComponent>())
                {
                    continue;
                }

                entt::entity l_Handle = l_Target.GetHandle();
                if (l_Handle == m_Context.SelectedEntity && l_PrimaryIsHandle)
                {
                    // The primary takes the gizmo's exact matrix so it never drifts.
                    l_ApplyWorld(l_Handle, l_World);
                }
                else
                {
                    // Everyone else receives the same world-space delta this frame.
                    l_ApplyWorld(l_Handle, l_Delta * l_Scene.GetWorldMatrix(l_Handle));
                }
            }
        }

        if (!l_IsUsing && m_GizmoWasUsing)
        {
            std::unique_ptr<CompositeCommand> l_Composite = std::make_unique<CompositeCommand>(m_GizmoStartTransforms.size() > 1 ? "Edit Transforms" : "Edit Transform");
            for (const auto& it_Start : m_GizmoStartTransforms)
            {
                Entity l_Target = FindEntityByUUID(l_Scene, it_Start.first);
                if (!l_Target.IsValid() || !l_Target.HasComponent<TransformComponent>())
                {
                    continue;
                }

                const TransformComponent& l_Current = l_Target.GetComponent<TransformComponent>();
                if (l_Current.Translation != it_Start.second.Translation || l_Current.Rotation != it_Start.second.Rotation || l_Current.Scale != it_Start.second.Scale)
                {
                    l_Composite->Add(std::make_unique<SetTransformCommand>(l_Scene, it_Start.first, it_Start.second, l_Current));
                }
            }

            if (!l_Composite->Empty())
            {
                m_Context.History.Execute(std::move(l_Composite));
            }

            m_GizmoStartTransforms.clear();
        }

        m_GizmoWasUsing = l_IsUsing;
    }

    void ViewportPanel::RenderToolsOverlay(const ImVec2& viewportMin)
    {
        const float l_Margin = 8.0f;
        float l_ButtonWidth = ImGui::GetFrameHeight() + 4.0f;

        // Unity stacks the tool handles vertically down the left edge of the Scene view.
        SceneOverlay l_Overlay = BeginSceneOverlay(ImVec2(viewportMin.x + l_Margin, viewportMin.y + l_Margin), ImVec2(4.0f, 4.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));

        float l_GripInset = (l_ButtonWidth - ImGui::CalcTextSize(ICON_TR_GRIP).x) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (l_GripInset > 0.0f ? l_GripInset : 0.0f));
        ImGui::TextDisabled(ICON_TR_GRIP);

        auto l_ToolButton = [&](const char* icon, SceneTool tool, const char* tooltip)
        {
            if (EditorTheme::ToolBarButton(icon, m_Tool == tool, tooltip, true, l_ButtonWidth, EditorColors::Highlight))
            {
                m_Tool = tool;
            }
        };

        l_ToolButton(ICON_TR_TOOL_VIEW, SceneTool::View, "View Tool  (Q)");
        l_ToolButton(ICON_TR_TOOL_MOVE, SceneTool::Move, "Move Tool  (W)");
        l_ToolButton(ICON_TR_TOOL_ROTATE, SceneTool::Rotate, "Rotate Tool  (E)");
        l_ToolButton(ICON_TR_TOOL_SCALE, SceneTool::Scale, "Scale Tool  (R)");
        l_ToolButton(ICON_TR_TOOL_TRANSFORM, SceneTool::Transform, "Transform Tool  (Y)");

        ImGui::PopStyleVar();

        EndSceneOverlay(l_Overlay);
    }

    void ViewportPanel::RenderOrientationOverlay(const ImVec2& viewportMin)
    {
        const float l_Margin = 8.0f;
        float l_ToolStripWidth = ImGui::GetFrameHeight() + 20.0f;
        float l_ButtonHeight = ImGui::GetFrameHeight();

        // Pivot / Center and Local / Global sit in their own strip beside the tool handles.
        SceneOverlay l_Overlay = BeginSceneOverlay(ImVec2(viewportMin.x + l_Margin + l_ToolStripWidth, viewportMin.y + l_Margin), ImVec2(5.0f, 4.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));

        if (ImGui::Button(m_PivotCenter ? ICON_TR_CENTER "  Center" : ICON_TR_PIVOT "  Pivot", ImVec2(ImGui::GetFontSize() * 5.5f, l_ButtonHeight)))
        {
            m_PivotCenter = !m_PivotCenter;
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Handle position: Pivot / Center");
        }

        ImGui::SameLine();
        if (ImGui::Button(m_GizmoMode == ImGuizmo::LOCAL ? ICON_TR_LOCAL "  Local" : ICON_TR_GLOBAL "  Global", ImVec2(ImGui::GetFontSize() * 5.5f, l_ButtonHeight)))
        {
            m_GizmoMode = m_GizmoMode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Handle rotation: Local / Global");
        }

        ImGui::SameLine();
        if (EditorTheme::ToolBarButton(m_SnapEnabled ? ICON_TR_SNAP : ICON_MDI_GRID_OFF, m_SnapEnabled, m_SnapEnabled ? "Grid Snapping: On" : "Grid Snapping: Off", true, 0.0f, EditorColors::Highlight))
        {
            m_SnapEnabled = !m_SnapEnabled;
        }

        // The snap increment tracks whichever tool is active, exactly as Unity's Increment Snapping does.
        float* l_SnapField = &m_SnapTranslation;
        const char* l_SnapFormat = "%.2f";
        if (m_Tool == SceneTool::Rotate)
        {
            l_SnapField = &m_SnapRotation;
            l_SnapFormat = "%.0f";
        }
        else if (m_Tool == SceneTool::Scale)
        {
            l_SnapField = &m_SnapScale;
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 3.5f);
        ImGui::DragFloat("##SceneSnapValue", l_SnapField, 0.05f, 0.0f, 1000.0f, l_SnapFormat);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Snap increment");
        }

        if (m_Engine.HasEditorCamera())
        {
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled(ICON_TR_CAMERA);

            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 3.5f);

            EditorCamera& l_Camera = m_Engine.GetEditorCameraController();
            float l_Speed = l_Camera.GetMoveSpeed();
            if (ImGui::DragFloat("##SceneCameraSpeed", &l_Speed, 0.1f, 0.1f, 100.0f, "%.1f"))
            {
                l_Camera.SetMoveSpeed(l_Speed);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Camera fly speed (scroll wheel while flying)");
            }
        }

        ImGui::SameLine();
        if (EditorTheme::ToolBarButton(ICON_TR_STATS, m_ShowStats, "Statistics", true, 0.0f, EditorColors::Highlight))
        {
            m_ShowStats = !m_ShowStats;
        }

        ImGui::PopStyleVar();

        EndSceneOverlay(l_Overlay);
    }

    void ViewportPanel::RenderStatsOverlay(const ImVec2& viewportMin, const ImVec2& viewportSize)
    {
        if (!m_ShowStats)
        {
            return;
        }

        const float l_Margin = 12.0f;
        const float l_Pad = 6.0f;

        ImGuiIO& l_IO = ImGui::GetIO();
        float l_FrameMs = l_IO.Framerate > 0.0f ? 1000.0f / l_IO.Framerate : 0.0f;

        std::vector<std::pair<std::string, std::string>> l_Rows;
        char l_Buffer[64];

        std::snprintf(l_Buffer, sizeof(l_Buffer), "%.1f", l_IO.Framerate);
        l_Rows.emplace_back("FPS", l_Buffer);

        std::snprintf(l_Buffer, sizeof(l_Buffer), "%.2f ms", l_FrameMs);
        l_Rows.emplace_back("Frame", l_Buffer);

        if (m_Engine.HasScene())
        {
            std::snprintf(l_Buffer, sizeof(l_Buffer), "%d", static_cast<int>(m_Engine.GetScene().GetRegistry().view<IDComponent>().size()));
            l_Rows.emplace_back("Entities", l_Buffer);
        }

        if (m_Engine.HasRenderer())
        {
            const RenderStats& l_Stats = m_Engine.GetRenderer().GetStats();

            std::snprintf(l_Buffer, sizeof(l_Buffer), "%u", l_Stats.Meshes);
            l_Rows.emplace_back("Meshes", l_Buffer);

            std::snprintf(l_Buffer, sizeof(l_Buffer), "%u", l_Stats.DrawCalls);
            l_Rows.emplace_back("Draw Calls", l_Buffer);

            std::snprintf(l_Buffer, sizeof(l_Buffer), "%u", l_Stats.ShadowDrawCalls);
            l_Rows.emplace_back("Shadow Draws", l_Buffer);

            std::snprintf(l_Buffer, sizeof(l_Buffer), "%u", l_Stats.Triangles);
            l_Rows.emplace_back("Triangles", l_Buffer);
        }

        float l_LineHeight = ImGui::GetTextLineHeightWithSpacing();
        float l_PanelWidth = ImGui::GetFontSize() * 11.0f;
        float l_PanelHeight = static_cast<float>(l_Rows.size() + 1) * l_LineHeight + l_Pad * 2.0f - ImGui::GetStyle().ItemSpacing.y;

        ImVec2 l_PanelMax = ImVec2(viewportMin.x + viewportSize.x - l_Margin, viewportMin.y + l_Margin + l_PanelHeight);
        ImVec2 l_PanelMin = ImVec2(l_PanelMax.x - l_PanelWidth, viewportMin.y + l_Margin);

        // Pure draw-list overlay (no widgets), matching Unity's Statistics window.
        ImDrawList* l_DrawList = ImGui::GetWindowDrawList();
        l_DrawList->AddRectFilled(l_PanelMin, l_PanelMax, EditorColors::OverlayBackground, 4.0f);
        l_DrawList->AddRect(l_PanelMin, l_PanelMax, EditorColors::DefaultBorder, 4.0f);
        l_DrawList->AddRectFilled(l_PanelMin, ImVec2(l_PanelMax.x, l_PanelMin.y + l_LineHeight), EditorColors::Toolbar, 4.0f, ImDrawFlags_RoundCornersTop);
        l_DrawList->AddText(ImVec2(l_PanelMin.x + l_Pad, l_PanelMin.y), EditorColors::DefaultText, "Statistics");

        ImU32 l_LabelColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        ImU32 l_ValueColor = ImGui::GetColorU32(ImGuiCol_Text);

        float l_Y = l_PanelMin.y + l_LineHeight + l_Pad;
        for (const auto& it_Row : l_Rows)
        {
            l_DrawList->AddText(ImVec2(l_PanelMin.x + l_Pad, l_Y), l_LabelColor, it_Row.first.c_str());
            ImVec2 l_ValueSize = ImGui::CalcTextSize(it_Row.second.c_str());
            l_DrawList->AddText(ImVec2(l_PanelMax.x - l_Pad - l_ValueSize.x, l_Y), l_ValueColor, it_Row.second.c_str());
            l_Y += l_LineHeight;
        }
    }

    void ViewportPanel::FocusOnSelection()
    {
        if (!m_Engine.HasScene() || !m_Engine.HasEditorCamera() || m_Context.SelectedEntity == entt::null)
        {
            return;
        }

        Scene& l_Scene = m_Engine.GetScene();
        if (!l_Scene.GetRegistry().valid(m_Context.SelectedEntity))
        {
            return;
        }

        glm::mat4 l_World = l_Scene.GetWorldMatrix(m_Context.SelectedEntity);
        glm::vec3 l_Target = glm::vec3(l_World[3]);

        // Meshes carry no bounds yet, so approximate the entity's world size from the matrix axes.
        float l_Radius = glm::max(glm::length(glm::vec3(l_World[0])), glm::max(glm::length(glm::vec3(l_World[1])), glm::length(glm::vec3(l_World[2]))));
        float l_Distance = glm::max(l_Radius * 3.0f, 2.0f);

        m_Engine.GetEditorCameraController().Focus(l_Target, l_Distance);
    }
}