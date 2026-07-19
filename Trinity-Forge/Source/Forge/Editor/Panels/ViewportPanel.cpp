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

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>
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

    void ViewportPanel::OnImGuiRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");

        m_Focused = ImGui::IsWindowFocused();
        m_Hovered = ImGui::IsWindowHovered();

        m_Engine.SetViewportInteractive(m_Hovered);

        if (m_Hovered && !ImGui::GetIO().WantTextInput)
        {
            Input& l_Input = m_Engine.GetPlatform().GetInput();
            if (!l_Input.IsMouseButtonPressed(Mouse::ButtonRight))
            {
                if (l_Input.IsKeyPressed(Key::W))
                {
                    m_GizmoOperation = ImGuizmo::TRANSLATE;
                }

                if (l_Input.IsKeyPressed(Key::E))
                {
                    m_GizmoOperation = ImGuizmo::ROTATE;
                }

                if (l_Input.IsKeyPressed(Key::R))
                {
                    m_GizmoOperation = ImGuizmo::SCALE;
                }

                if (l_Input.IsKeyPressed(Key::F))
                {
                    FocusOnSelection();
                }
            }
            else
            {
                // Flying (RMB held): the mouse wheel scales the camera speed, Unreal-style.
                float l_Wheel = ImGui::GetIO().MouseWheel;
                if (l_Wheel != 0.0f && m_Engine.HasEditorCamera())
                {
                    EditorCamera& l_Camera = m_Engine.GetEditorCameraController();
                    l_Camera.SetMoveSpeed(l_Camera.GetMoveSpeed() * std::pow(1.15f, l_Wheel));
                }
            }
        }

        ImVec2 l_Available = ImGui::GetContentRegionAvail();
        uint32_t l_Width = l_Available.x > 0.0f ? static_cast<uint32_t>(l_Available.x) : 0;
        uint32_t l_Height = l_Available.y > 0.0f ? static_cast<uint32_t>(l_Available.y) : 0;

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
                RenderOverlayToolbar(l_ImageMin);
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
        if (!m_Engine.HasScene() || m_Context.SelectedEntity == entt::null)
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
        glm::mat4 l_World = l_Scene.GetWorldMatrix(m_Context.SelectedEntity);
        glm::mat4 l_WorldBefore = l_World;

        float l_SnapValues[3] = { 0.0f, 0.0f, 0.0f };
        float* l_Snap = nullptr;
        if (m_SnapEnabled)
        {
            float l_Amount = m_SnapTranslation;
            if (m_GizmoOperation == ImGuizmo::ROTATE)
            {
                l_Amount = m_SnapRotation;
            }
            else if (m_GizmoOperation == ImGuizmo::SCALE)
            {
                l_Amount = m_SnapScale;
            }

            l_SnapValues[0] = l_SnapValues[1] = l_SnapValues[2] = l_Amount;
            l_Snap = l_SnapValues;
        }

        ImGuizmo::Manipulate(glm::value_ptr(l_View), glm::value_ptr(l_Projection), m_GizmoOperation, m_GizmoMode, glm::value_ptr(l_World), nullptr, l_Snap);

        bool l_IsUsing = ImGuizmo::IsUsing();

        if (l_IsUsing)
        {
            if (!m_GizmoWasUsing)
            {
                // Snapshot the top-most selected transforms as undo baselines; children of
                // selected parents follow through the hierarchy and are not written directly.
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
                    l_Transform.Rotation = glm::radians(glm::vec3(l_Rotation[0], l_Rotation[1], l_Rotation[2]));
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
                if (l_Handle == m_Context.SelectedEntity)
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

    void ViewportPanel::RenderOverlayToolbar(const ImVec2& viewportMin)
    {
        const float l_Margin = 12.0f;
        const float l_Pad = 6.0f;
        float l_ButtonSize = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        const ImVec4 l_Accent = ImVec4(0.16f, 0.30f, 0.49f, 1.0f);
        ImU32 l_SeparatorColor = ImGui::GetColorU32(ImGuiCol_Separator);

        // Split so the rounded panel can be drawn behind the controls after they are measured.
        ImDrawList* l_DrawList = ImGui::GetWindowDrawList();
        l_DrawList->ChannelsSplit(2);
        l_DrawList->ChannelsSetCurrent(1);

        ImGui::SetCursorScreenPos(ImVec2(viewportMin.x + l_Margin + l_Pad, viewportMin.y + l_Margin + l_Pad));
        ImGui::BeginGroup();

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        auto l_ModeButton = [&](const char* icon, ImGuizmo::OPERATION op, const char* tooltip)
            {
                bool l_Active = m_GizmoOperation == op;
                if (l_Active)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, l_Accent);
                }

                if (ImGui::Button(icon, ImVec2(l_ButtonSize, l_ButtonSize)))
                {
                    m_GizmoOperation = op;
                }

                if (l_Active)
                {
                    ImGui::PopStyleColor();
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", tooltip);
                }
            };

        auto l_VerticalSeparator = [&]()
            {
                ImGui::SameLine(0.0f, 7.0f);
                ImVec2 l_Pos = ImGui::GetCursorScreenPos();
                l_DrawList->AddLine(ImVec2(l_Pos.x, l_Pos.y + 3.0f), ImVec2(l_Pos.x, l_Pos.y + l_ButtonSize - 3.0f), l_SeparatorColor);
                ImGui::SameLine(0.0f, 7.0f);
            };

        l_ModeButton(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, ImGuizmo::TRANSLATE, "Move (W)");
        ImGui::SameLine();
        l_ModeButton(ICON_FA_ROTATE, ImGuizmo::ROTATE, "Rotate (E)");
        ImGui::SameLine();
        l_ModeButton(ICON_FA_MAXIMIZE, ImGuizmo::SCALE, "Scale (R)");

        l_VerticalSeparator();

        const char* l_SpaceLabel = m_GizmoMode == ImGuizmo::LOCAL ? "Local" : "World";
        if (ImGui::Button(l_SpaceLabel, ImVec2(0.0f, l_ButtonSize)))
        {
            m_GizmoMode = m_GizmoMode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Transform space (World / Local)");
        }

        l_VerticalSeparator();

        bool l_SnapActive = m_SnapEnabled;
        if (l_SnapActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, l_Accent);
        }

        if (ImGui::Button(ICON_FA_MAGNET, ImVec2(l_ButtonSize, l_ButtonSize)))
        {
            m_SnapEnabled = !m_SnapEnabled;
        }

        if (l_SnapActive)
        {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(m_SnapEnabled ? "Grid Snapping: On" : "Grid Snapping: Off");
        }

        // The snap increment field tracks whichever transform mode is active.
        float* l_SnapField = &m_SnapTranslation;
        const char* l_SnapFormat = "%.2f";
        if (m_GizmoOperation == ImGuizmo::ROTATE)
        {
            l_SnapField = &m_SnapRotation;
            l_SnapFormat = "%.0f";
        }
        else if (m_GizmoOperation == ImGuizmo::SCALE)
        {
            l_SnapField = &m_SnapScale;
        }

        ImGui::SameLine(0.0f, 4.0f);
        ImGui::SetNextItemWidth(60.0f);
        ImGui::DragFloat("##ViewportSnapValue", l_SnapField, 0.05f, 0.0f, 1000.0f, l_SnapFormat);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Snap increment");
        }

        l_VerticalSeparator();

        if (m_Engine.HasEditorCamera())
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(ICON_FA_CAMERA);
            ImGui::SameLine(0.0f, 4.0f);
            ImGui::SetNextItemWidth(60.0f);

            EditorCamera& l_Camera = m_Engine.GetEditorCameraController();
            float l_Speed = l_Camera.GetMoveSpeed();
            if (ImGui::DragFloat("##ViewportCameraSpeed", &l_Speed, 0.1f, 0.1f, 100.0f, "%.1f"))
            {
                l_Camera.SetMoveSpeed(l_Speed);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Camera fly speed (scroll wheel while flying)");
            }

            l_VerticalSeparator();
        }

        bool l_StatsActive = m_ShowStats;
        if (l_StatsActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, l_Accent);
        }

        if (ImGui::Button(ICON_FA_CIRCLE_INFO, ImVec2(l_ButtonSize, l_ButtonSize)))
        {
            m_ShowStats = !m_ShowStats;
        }

        if (l_StatsActive)
        {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(m_ShowStats ? "Viewport Statistics: On" : "Viewport Statistics: Off");
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImGui::EndGroup();

        ImVec2 l_Min = ImGui::GetItemRectMin();
        ImVec2 l_Max = ImGui::GetItemRectMax();

        l_DrawList->ChannelsSetCurrent(0);
        l_DrawList->AddRectFilled(ImVec2(l_Min.x - l_Pad, l_Min.y - l_Pad), ImVec2(l_Max.x + l_Pad, l_Max.y + l_Pad), IM_COL32(18, 18, 18, 220), 5.0f);
        l_DrawList->AddRect(ImVec2(l_Min.x - l_Pad, l_Min.y - l_Pad), ImVec2(l_Max.x + l_Pad, l_Max.y + l_Pad), IM_COL32(70, 70, 70, 160), 5.0f);
        l_DrawList->ChannelsMerge();
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
        float l_PanelHeight = static_cast<float>(l_Rows.size()) * l_LineHeight + l_Pad * 2.0f - ImGui::GetStyle().ItemSpacing.y;

        ImVec2 l_PanelMax = ImVec2(viewportMin.x + viewportSize.x - l_Margin, viewportMin.y + l_Margin + l_PanelHeight);
        ImVec2 l_PanelMin = ImVec2(l_PanelMax.x - l_PanelWidth, viewportMin.y + l_Margin);

        // Pure draw-list overlay (no widgets), styled to match the transform toolbar's panel.
        ImDrawList* l_DrawList = ImGui::GetWindowDrawList();
        l_DrawList->AddRectFilled(l_PanelMin, l_PanelMax, IM_COL32(18, 18, 18, 220), 5.0f);
        l_DrawList->AddRect(l_PanelMin, l_PanelMax, IM_COL32(70, 70, 70, 160), 5.0f);

        ImU32 l_LabelColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        ImU32 l_ValueColor = ImGui::GetColorU32(ImGuiCol_Text);

        float l_Y = l_PanelMin.y + l_Pad;
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