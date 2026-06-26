#include <Forge/Editor/Panels/HierarchyPanel.h>

#include <algorithm>
#include <cctype>
#include <string>

#include <imgui.h>
#include <imgui_internal.h>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/NameComponent.h>
#include <Trinity/Scene/Components/HierarchyComponent.h>
#include <Trinity/Scene/Components/CameraComponent.h>
#include <Trinity/Scene/Components/LightComponent.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Scene/Components/AudioSourceComponent.h>
#include <Trinity/Scene/Components/AudioListenerComponent.h>

namespace Trinity
{
    static std::string ToLower(const std::string& text)
    {
        std::string l_Result = text;
        std::transform(l_Result.begin(), l_Result.end(), l_Result.begin(), [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });

        return l_Result;
    }

    static const char* EntityIcon(entt::registry& registry, entt::entity entity)
    {
        if (registry.all_of<CameraComponent>(entity))
        {
            return ICON_FA_CAMERA;
        }

        if (registry.all_of<LightComponent>(entity))
        {
            return ICON_FA_LIGHTBULB;
        }

        if (registry.all_of<AudioSourceComponent>(entity) || registry.all_of<AudioListenerComponent>(entity))
        {
            return ICON_FA_VOLUME_HIGH;
        }

        if (registry.all_of<MeshRendererComponent>(entity))
        {
            return ICON_FA_CUBE;
        }

        return nullptr;
    }

    static const char* EntityTypeLabel(entt::registry& registry, entt::entity entity)
    {
        if (registry.all_of<CameraComponent>(entity))
        {
            return "Camera";
        }

        if (registry.all_of<LightComponent>(entity))
        {
            return "Light";
        }

        if (registry.all_of<AudioSourceComponent>(entity))
        {
            return "Audio Source";
        }

        if (registry.all_of<AudioListenerComponent>(entity))
        {
            return "Audio Listener";
        }

        if (registry.all_of<MeshRendererComponent>(entity))
        {
            return "Mesh";
        }

        return "Entity";
    }

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

            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputTextWithHint("##HierarchySearch", ICON_FA_SLIDERS "  Search...", m_SearchBuffer, sizeof(m_SearchBuffer));
            ImGui::Separator();

            std::string l_Filter = ToLower(m_SearchBuffer);

            if (!l_Filter.empty())
            {
                auto l_View = l_Registry.view<NameComponent>();
                for (entt::entity it_Entity : l_View)
                {
                    std::string l_NameLower = ToLower(l_Registry.get<NameComponent>(it_Entity).Name);
                    if (l_NameLower.find(l_Filter) != std::string::npos)
                    {
                        RenderFlatRow(l_Scene, it_Entity);
                    }
                }
            }
            else
            {
                auto l_View = l_Registry.view<NameComponent>();
                for (entt::entity it_Entity : l_View)
                {
                    const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(it_Entity);
                    if (l_Hierarchy == nullptr || l_Hierarchy->Parent == entt::null)
                    {
                        RenderEntityNode(l_Scene, it_Entity);
                    }
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

        const char* l_Icon = EntityIcon(l_Registry, entity);
        std::string l_Label = l_Icon != nullptr ? (std::string(l_Icon) + "  " + l_Name) : l_Name;

        ImGuiTreeNodeFlags l_Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!l_HasChildren)
        {
            l_Flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (m_Context.SelectedEntity == entity)
        {
            l_Flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool l_Open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(entity))), l_Flags, "%s", l_Label.c_str());

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

        const char* l_Type = EntityTypeLabel(l_Registry, entity);
        float l_TypeWidth = ImGui::CalcTextSize(l_Type).x;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - l_TypeWidth);
        ImGui::TextDisabled("%s", l_Type);

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

    void HierarchyPanel::RenderFlatRow(Scene& scene, entt::entity entity)
    {
        entt::registry& l_Registry = scene.GetRegistry();
        const std::string& l_Name = l_Registry.get<NameComponent>(entity).Name;

        const char* l_Icon = EntityIcon(l_Registry, entity);
        std::string l_Label = l_Icon != nullptr ? (std::string(l_Icon) + "  " + l_Name) : l_Name;

        bool l_Selected = m_Context.SelectedEntity == entity;
        if (ImGui::Selectable(l_Label.c_str(), l_Selected, ImGuiSelectableFlags_SpanAvailWidth))
        {
            m_Context.SelectedEntity = entity;
        }

        if (ImGui::BeginPopupContextItem())
        {
            m_Context.SelectedEntity = entity;

            if (ImGui::MenuItem("Duplicate"))
            {
                m_Context.Action = PendingAction::Duplicate;
                m_Context.ActionTarget = entity;
            }

            if (ImGui::MenuItem("Delete"))
            {
                m_Context.Action = PendingAction::Delete;
                m_Context.ActionTarget = entity;
            }

            ImGui::EndPopup();
        }

        const char* l_Type = EntityTypeLabel(l_Registry, entity);
        float l_TypeWidth = ImGui::CalcTextSize(l_Type).x;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - l_TypeWidth);
        ImGui::TextDisabled("%s", l_Type);
    }
}