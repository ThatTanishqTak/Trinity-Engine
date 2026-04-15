#include "Forge/Panels/SceneHierarchyPanel.h"

#include "Trinity/Scene/Scene.h"
#include "Trinity/Scene/Entity.h"
#include "Trinity/Scene/Components/TagComponent.h"

#include <imgui.h>

#include <cstring>

namespace Forge
{
    SceneHierarchyPanel::SceneHierarchyPanel(std::string name, SelectionContext* context)
        : Panel(std::move(name)), m_Context(context)
    {
    }

    void SceneHierarchyPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        Trinity::Scene* l_Scene = m_Context->ActiveScene;

        if (l_Scene)
        {
            // Iterate all entities that have a TagComponent (every entity created via Scene::CreateEntity)
            auto l_View = l_Scene->GetRegistry().view<Trinity::TagComponent>();
            for (auto l_Entity : l_View)
            {
                RenderEntityNode(l_Entity);
            }

            // Right-click on empty panel area to spawn
            if (ImGui::BeginPopupContextWindow("##HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::MenuItem("Create Empty Entity"))
                {
                    l_Scene->CreateEntity("Entity");
                }

                ImGui::EndPopup();
            }

            // Deselect by clicking empty space
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_Context->SelectedEntity = entt::null;
            }
        }

        ImGui::End();
    }

    void SceneHierarchyPanel::RenderEntityNode(entt::entity entity)
    {
        Trinity::Scene* l_Scene = m_Context->ActiveScene;
        auto& l_Tag = l_Scene->GetRegistry().get<Trinity::TagComponent>(entity);

        const bool l_Selected = (m_Context->SelectedEntity == entity);

        ImGuiTreeNodeFlags l_Flags = ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_Leaf;  // No children until Phase 18.2 parenting

        if (l_Selected)
            l_Flags |= ImGuiTreeNodeFlags_Selected;

        // Inline rename
        if (m_RenameTarget == entity && m_RenameRequested)
        {
            char l_Buffer[256];
            std::strncpy(l_Buffer, l_Tag.Tag.c_str(), sizeof(l_Buffer) - 1);
            l_Buffer[sizeof(l_Buffer) - 1] = '\0';

            ImGui::SetNextItemWidth(-1.0f);
            ImGui::SetKeyboardFocusHere();

            if (ImGui::InputText("##Rename", l_Buffer, sizeof(l_Buffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                l_Tag.Tag = l_Buffer;
                m_RenameTarget    = entt::null;
                m_RenameRequested = false;
            }

            if (!ImGui::IsItemActive() && !ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_RenameTarget    = entt::null;
                m_RenameRequested = false;
            }

            return;
        }

        const bool l_NodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(entity))), l_Flags, "%s", l_Tag.Tag.c_str());

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            m_Context->SelectedEntity = entity;
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Rename"))
            {
                m_Context->SelectedEntity = entity;
                m_RenameTarget    = entity;
                m_RenameRequested = true;
            }

            if (ImGui::MenuItem("Duplicate"))
            {
                Trinity::Entity l_New = l_Scene->CreateEntity(l_Tag.Tag);
                (void)l_New;
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Delete"))
            {
                if (m_Context->SelectedEntity == entity)
                    m_Context->SelectedEntity = entt::null;

                Trinity::Entity l_Wrapper(entity, l_Scene);
                l_Scene->DestroyEntity(l_Wrapper);

                if (l_NodeOpen)
                    ImGui::TreePop();

                ImGui::EndPopup();
                return;
            }

            ImGui::EndPopup();
        }

        if (l_NodeOpen)
            ImGui::TreePop();
    }
}
