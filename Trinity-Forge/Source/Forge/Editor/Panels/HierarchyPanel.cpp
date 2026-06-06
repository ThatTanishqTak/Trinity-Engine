#include <Forge/Editor/Panels/HierarchyPanel.h>

#include <string>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/NameComponent.h>
#include <Trinity/Scene/Components/HierarchyComponent.h>

namespace Trinity
{
    void HierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin("Hierarchy");

        if (m_Engine.HasScene())
        {
            Scene& l_Scene = m_Engine.GetScene();
            entt::registry& l_Registry = l_Scene.GetRegistry();

            if (m_Context.SelectedEntity != entt::null && !l_Registry.valid(m_Context.SelectedEntity))
            {
                m_Context.SelectedEntity = entt::null;
            }

            auto l_View = l_Registry.view<NameComponent>();
            for (entt::entity it_Entity : l_View)
            {
                const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(it_Entity);
                if (l_Hierarchy == nullptr || l_Hierarchy->Parent == entt::null)
                {
                    RenderEntityNode(l_Scene, it_Entity);
                }
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
            {
                m_Context.SelectedEntity = entt::null;
            }

            if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::MenuItem("Create Empty"))
                {
                    m_Context.Action = PendingAction::Create;
                }

                ImGui::EndPopup();
            }
        }

        ImGui::End();
    }

    void HierarchyPanel::RenderEntityNode(Scene& scene, entt::entity entity)
    {
        entt::registry& l_Registry = scene.GetRegistry();
        const std::string& l_Name = l_Registry.get<NameComponent>(entity).Name;

        const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(entity);
        bool l_HasChildren = l_Hierarchy != nullptr && !l_Hierarchy->Children.empty();

        ImGuiTreeNodeFlags l_Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!l_HasChildren)
        {
            l_Flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (m_Context.SelectedEntity == entity)
        {
            l_Flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool l_Open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(entity))), l_Flags, "%s", l_Name.c_str());

        if (ImGui::IsItemClicked())
        {
            m_Context.SelectedEntity = entity;
        }

        if (ImGui::BeginDragDropSource())
        {
            entt::entity l_Payload = entity;
            ImGui::SetDragDropPayload("TRINITY_ENTITY", &l_Payload, sizeof(l_Payload));
            ImGui::TextUnformatted(l_Name.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* l_Accepted = ImGui::AcceptDragDropPayload("TRINITY_ENTITY"))
            {
                entt::entity l_Dragged = *static_cast<const entt::entity*>(l_Accepted->Data);
                m_Context.Action = PendingAction::Reparent;
                m_Context.ActionTarget = l_Dragged;
                m_Context.ActionParent = entity;
            }

            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem())
        {
            m_Context.SelectedEntity = entity;

            if (ImGui::MenuItem("Duplicate"))
            {
                m_Context.Action = PendingAction::Duplicate;
                m_Context.ActionTarget = entity;
            }

            bool l_HasParent = l_Hierarchy != nullptr && l_Hierarchy->Parent != entt::null;
            if (ImGui::MenuItem("Unparent", nullptr, false, l_HasParent))
            {
                m_Context.Action = PendingAction::Reparent;
                m_Context.ActionTarget = entity;
                m_Context.ActionParent = entt::null;
            }

            if (ImGui::MenuItem("Delete"))
            {
                m_Context.Action = PendingAction::Delete;
                m_Context.ActionTarget = entity;
            }

            ImGui::EndPopup();
        }

        if (l_Open)
        {
            if (l_Hierarchy != nullptr)
            {
                for (entt::entity it_Child : l_Hierarchy->Children)
                {
                    RenderEntityNode(scene, it_Child);
                }
            }

            ImGui::TreePop();
        }
    }
}