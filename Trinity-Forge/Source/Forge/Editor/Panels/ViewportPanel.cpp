#include <Forge/Editor/Panels/ViewportPanel.h>

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/Commands/PropertyCommands.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Renderer/Frontend/Camera.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/HierarchyComponent.h>

namespace Trinity
{
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
                RenderGizmo(ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
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

        TransformComponent l_PreEdit = l_Registry.get<TransformComponent>(m_Context.SelectedEntity);

        ImGuizmo::Manipulate(glm::value_ptr(l_View), glm::value_ptr(l_Projection), m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(l_World));

        bool l_IsUsing = ImGuizmo::IsUsing();

        if (l_IsUsing)
        {
            if (!m_GizmoWasUsing)
            {
                m_GizmoStartTransform = l_PreEdit;
            }

            glm::mat4 l_ParentWorld(1.0f);
            const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(m_Context.SelectedEntity);
            if (l_Hierarchy != nullptr && l_Hierarchy->Parent != entt::null)
            {
                l_ParentWorld = l_Scene.GetWorldMatrix(l_Hierarchy->Parent);
            }

            glm::mat4 l_Local = glm::inverse(l_ParentWorld) * l_World;

            float l_Translation[3];
            float l_Rotation[3];
            float l_Scale[3];
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(l_Local), l_Translation, l_Rotation, l_Scale);

            TransformComponent& l_Transform = l_Registry.get<TransformComponent>(m_Context.SelectedEntity);
            l_Transform.Translation = glm::vec3(l_Translation[0], l_Translation[1], l_Translation[2]);
            l_Transform.Rotation = glm::radians(glm::vec3(l_Rotation[0], l_Rotation[1], l_Rotation[2]));
            l_Transform.Scale = glm::vec3(l_Scale[0], l_Scale[1], l_Scale[2]);
        }

        if (!l_IsUsing && m_GizmoWasUsing)
        {
            TransformComponent l_Current = l_Registry.get<TransformComponent>(m_Context.SelectedEntity);
            if (l_Current.Translation != m_GizmoStartTransform.Translation || l_Current.Rotation != m_GizmoStartTransform.Rotation || l_Current.Scale != m_GizmoStartTransform.Scale)
            {
                uint64_t l_UUID = static_cast<uint64_t>(Entity(m_Context.SelectedEntity, &l_Scene).GetUUID());
                m_Context.History.Execute(std::make_unique<SetTransformCommand>(l_Scene, l_UUID, m_GizmoStartTransform, l_Current));
            }
        }

        m_GizmoWasUsing = l_IsUsing;
    }
}