#include "Forge/Panels/SceneHierarchyPanel.h"

#include "Trinity/Scene/Scene.h"
#include "Trinity/Scene/Entity.h"
#include "Trinity/Scene/Components/UUIDComponent.h"
#include "Trinity/Scene/Components/TagComponent.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Scene/Components/MeshComponent.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Geometry/Geometry.h"
#include "Trinity/Asset/AssetRegistry.h"

#include <imgui.h>

#include <cstring>
#include <vector>

namespace Forge
{
    SceneHierarchyPanel::SceneHierarchyPanel(std::string name, SelectionContext* context) : Panel(std::move(name)), m_Context(context)
    {

    }

    void SceneHierarchyPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        if (m_Context->State != EditorState::Edit)
        {
            m_RenameTarget = entt::null;
            m_RenameRequested = false;
        }

        if (m_Context->ActiveScene)
        {
            auto a_View = m_Context->ActiveScene->GetRegistry().view<Trinity::UUIDComponent, Trinity::TransformComponent>();
            for (auto it_Entity : a_View)
            {
                const auto& a_Transform = a_View.get<Trinity::TransformComponent>(it_Entity);
                if (a_Transform.ParentUUID == 0)
                {
                    RenderEntityNode(it_Entity);
                }
            }

            if (m_Context->State == EditorState::Edit && ImGui::BeginPopupContextWindow("##HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::MenuItem("Create Empty Entity"))
                {
                    m_Context->ActiveScene->CreateEntity("Entity");
                }

                auto SpawnPrimitive = [&](const char* tag, Trinity::Geometry::PrimitiveType type)
                {
                    Trinity::Entity l_Entity = m_Context->ActiveScene->CreateEntity(tag);

                    const auto& l_Data = Trinity::Geometry::GetPrimitive(type);
                    auto a_Mesh = Trinity::Mesh::Create(l_Data.Vertices, l_Data.Indices);

                    auto& a_MeshComponent = l_Entity.AddComponent<Trinity::MeshComponent>();
                    a_MeshComponent.MeshAssetUUID = Trinity::AssetRegistry::Get().RegisterMesh(a_Mesh);
                    a_MeshComponent.MeshData = std::move(a_Mesh);
                };

                if (ImGui::MenuItem("Create Triangle"))
                {
                    SpawnPrimitive("Triangle", Trinity::Geometry::PrimitiveType::Triangle);
                }

                if (ImGui::MenuItem("Create Quad"))
                {
                    SpawnPrimitive("Quad", Trinity::Geometry::PrimitiveType::Quad);
                }

                if (ImGui::MenuItem("Create Cube"))
                {
                    SpawnPrimitive("Cube", Trinity::Geometry::PrimitiveType::Cube);
                }

                ImGui::EndPopup();
            }

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_Context->SelectedEntity = entt::null;
            }
        }

        ImGui::End();
    }

    void SceneHierarchyPanel::RenderEntityNode(entt::entity entity)
    {
        auto& a_Registry = m_Context->ActiveScene->GetRegistry();
        auto& a_Tag = a_Registry.get<Trinity::TagComponent>(entity);

        const uint64_t l_EntityUUID = a_Registry.get<Trinity::UUIDComponent>(entity).UUID;

        std::vector<entt::entity> l_Children;
        auto a_AllView = a_Registry.view<Trinity::UUIDComponent, Trinity::TransformComponent>();
        for (auto it_Child : a_AllView)
        {
            if (it_Child == entity)
            {
                continue;
            }

            if (a_AllView.get<Trinity::TransformComponent>(it_Child).ParentUUID == l_EntityUUID)
            {
                l_Children.push_back(it_Child);
            }
        }

        ImGuiTreeNodeFlags l_Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!l_Children.empty())
        {
            l_Flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (m_Context->SelectedEntity == entity)
        {
            l_Flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (m_RenameTarget == entity && m_RenameRequested && (m_Context->State == EditorState::Edit))
        {
            char l_Buffer[256];
            std::strncpy(l_Buffer, a_Tag.Tag.c_str(), sizeof(l_Buffer) - 1);
            l_Buffer[sizeof(l_Buffer) - 1] = '\0';

            ImGui::SetNextItemWidth(-1.0f);
            ImGui::SetKeyboardFocusHere();

            if (ImGui::InputText("##Rename", l_Buffer, sizeof(l_Buffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                a_Tag.Tag = l_Buffer;
                m_RenameTarget = entt::null;
                m_RenameRequested = false;
            }

            if (!ImGui::IsItemActive() && !ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_RenameTarget = entt::null;
                m_RenameRequested = false;
            }

            return;
        }

        const bool l_NodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(entity))), l_Flags, "%s", a_Tag.Tag.c_str());

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            m_Context->SelectedEntity = entity;
        }

        if (m_Context->State == EditorState::Edit && ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Rename"))
            {
                m_Context->SelectedEntity = entity;
                m_RenameTarget = entity;
                m_RenameRequested = true;
            }

            if (ImGui::MenuItem("Duplicate"))
            {
                Trinity::Entity l_New = m_Context->ActiveScene->CreateEntity(a_Tag.Tag);
                (void)l_New;
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Delete"))
            {
                if (m_Context->SelectedEntity == entity)
                {
                    m_Context->SelectedEntity = entt::null;
                }

                Trinity::Entity l_Wrapper(entity, m_Context->ActiveScene);
                m_Context->ActiveScene->DestroyEntity(l_Wrapper);

                ImGui::EndPopup();

                if (l_NodeOpen)
                {
                    ImGui::TreePop();
                }

                return;
            }

            ImGui::EndPopup();
        }

        if (l_NodeOpen)
        {
            for (auto it_Child : l_Children)
            {
                RenderEntityNode(it_Child);
            }

            ImGui::TreePop();
        }
    }
}