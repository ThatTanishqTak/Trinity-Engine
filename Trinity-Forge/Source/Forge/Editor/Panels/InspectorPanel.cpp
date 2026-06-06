#include <Forge/Editor/Panels/InspectorPanel.h>

#include <cstring>
#include <memory>
#include <string>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/Commands/EntityCommands.h>
#include <Forge/Editor/Commands/ComponentCommands.h>
#include <Forge/Editor/Commands/PropertyCommands.h>
#include <Forge/Editor/Commands/MeshCommands.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/NameComponent.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Scene/Components/CameraComponent.h>
#include <Trinity/Assets/AssetDatabase.h>

namespace Trinity
{
    void InspectorPanel::OnImGuiRender()
    {
        ImGui::Begin("Inspector");

        if (m_Engine.HasScene() && m_Context.SelectedEntity != entt::null)
        {
            Scene& l_Scene = m_Engine.GetScene();
            if (l_Scene.GetRegistry().valid(m_Context.SelectedEntity))
            {
                Entity l_Entity(m_Context.SelectedEntity, &l_Scene);

                NameComponent& l_NameComponent = l_Entity.GetComponent<NameComponent>();
                char l_NameBuffer[256];
                std::strncpy(l_NameBuffer, l_NameComponent.Name.c_str(), sizeof(l_NameBuffer) - 1);
                l_NameBuffer[sizeof(l_NameBuffer) - 1] = '\0';

                bool l_NameChanged = ImGui::InputText("Name", l_NameBuffer, sizeof(l_NameBuffer));
                if (ImGui::IsItemActivated())
                {
                    m_RenameOldName = l_NameComponent.Name;
                }

                if (l_NameChanged)
                {
                    l_NameComponent.Name = l_NameBuffer;
                }

                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (l_NameComponent.Name != m_RenameOldName)
                    {
                        m_Context.History.Execute(std::make_unique<RenameEntityCommand>(l_Scene, static_cast<uint64_t>(l_Entity.GetUUID()), m_RenameOldName, l_NameComponent.Name));
                    }
                }
                else if (ImGui::IsItemDeactivated())
                {
                    l_NameComponent.Name = m_RenameOldName;
                }

                if (l_Entity.HasComponent<TransformComponent>())
                {
                    ImGui::SeparatorText("Transform");
                    TransformComponent& l_Transform = l_Entity.GetComponent<TransformComponent>();
                    uint64_t l_TransformUUID = static_cast<uint64_t>(l_Entity.GetUUID());
                    DragTransformField(l_Scene, l_TransformUUID, "Translation", l_Transform, l_Transform.Translation, 0.1f);
                    DragTransformField(l_Scene, l_TransformUUID, "Rotation", l_Transform, l_Transform.Rotation, 0.01f);
                    DragTransformField(l_Scene, l_TransformUUID, "Scale", l_Transform, l_Transform.Scale, 0.1f);
                }

                if (l_Entity.HasComponent<MeshRendererComponent>())
                {
                    ImGui::SeparatorText("Mesh Renderer");
                    MeshRendererComponent& l_MeshRenderer = l_Entity.GetComponent<MeshRendererComponent>();
                    uint64_t l_MeshUUID = static_cast<uint64_t>(l_Entity.GetUUID());
                    AssetDatabase& l_Assets = m_Engine.GetAssetDatabase();

                    uint64_t l_CurrentAsset = static_cast<uint64_t>(l_MeshRenderer.MeshAsset);
                    std::string l_CurrentLabel;
                    if (l_CurrentAsset == 0)
                    {
                        l_CurrentLabel = "(none)";
                    }
                    else if (l_CurrentAsset == AssetDatabase::BuiltinCube)
                    {
                        l_CurrentLabel = "(procedural cube)";
                    }
                    else
                    {
                        const AssetMetadata* l_Meta = l_Assets.GetMetadata(l_MeshRenderer.MeshAsset);
                        l_CurrentLabel = l_Meta != nullptr ? l_Meta->SourcePath : "(missing)";
                    }

                    if (ImGui::BeginCombo("Mesh", l_CurrentLabel.c_str()))
                    {
                        if (ImGui::Selectable("(none)", l_CurrentAsset == 0) && l_CurrentAsset != 0)
                        {
                            m_Context.History.Execute(std::make_unique<SetMeshCommand>(l_Scene, l_Assets, l_MeshUUID, UUID(0)));
                        }

                        if (ImGui::Selectable("(procedural cube)", l_CurrentAsset == AssetDatabase::BuiltinCube) && l_CurrentAsset != AssetDatabase::BuiltinCube)
                        {
                            m_Context.History.Execute(std::make_unique<SetMeshCommand>(l_Scene, l_Assets, l_MeshUUID, UUID(AssetDatabase::BuiltinCube)));
                        }

                        for (UUID it_Asset : l_Assets.GetAssetsOfType(AssetType::Mesh))
                        {
                            const AssetMetadata* l_Meta = l_Assets.GetMetadata(it_Asset);
                            if (l_Meta == nullptr)
                            {
                                continue;
                            }

                            bool l_Selected = static_cast<uint64_t>(it_Asset) == l_CurrentAsset;
                            if (ImGui::Selectable(l_Meta->SourcePath.c_str(), l_Selected) && !l_Selected)
                            {
                                m_Context.History.Execute(std::make_unique<SetMeshCommand>(l_Scene, l_Assets, l_MeshUUID, it_Asset));
                            }
                        }

                        ImGui::EndCombo();
                    }

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* l_Accepted = ImGui::AcceptDragDropPayload("TRINITY_ASSET_MESH"))
                        {
                            uint64_t l_Dropped = *static_cast<const uint64_t*>(l_Accepted->Data);
                            m_Context.History.Execute(std::make_unique<SetMeshCommand>(l_Scene, l_Assets, l_MeshUUID, UUID(l_Dropped)));
                        }

                        ImGui::EndDragDropTarget();
                    }

                    if (ImGui::SmallButton("Remove##MeshRenderer"))
                    {
                        m_Context.ComponentOp = [this, l_MeshUUID]()
                        {
                            Scene& l_OpScene = m_Engine.GetScene();
                            m_Context.History.Execute(std::make_unique<RemoveComponentCommand<MeshRendererComponent>>(l_OpScene, l_MeshUUID, "Mesh Renderer"));
                        };
                    }
                }

                if (l_Entity.HasComponent<CameraComponent>())
                {
                    ImGui::SeparatorText("Camera");
                    CameraComponent& l_Camera = l_Entity.GetComponent<CameraComponent>();
                    if (ImGui::Checkbox("Primary", &l_Camera.Primary))
                    {
                        m_Context.History.Execute(std::make_unique<SetCameraPrimaryCommand>(l_Scene, static_cast<uint64_t>(l_Entity.GetUUID()), !l_Camera.Primary, l_Camera.Primary));
                    }

                    if (ImGui::SmallButton("Remove##Camera"))
                    {
                        uint64_t l_TargetUUID = static_cast<uint64_t>(l_Entity.GetUUID());
                        m_Context.ComponentOp = [this, l_TargetUUID]()
                        {
                            Scene& l_OpScene = m_Engine.GetScene();
                            m_Context.History.Execute(std::make_unique<RemoveComponentCommand<CameraComponent>>(l_OpScene, l_TargetUUID, "Camera"));
                        };
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Add Component"))
                {
                    ImGui::OpenPopup("AddComponentPopup");
                }

                if (ImGui::BeginPopup("AddComponentPopup"))
                {
                    uint64_t l_TargetUUID = static_cast<uint64_t>(l_Entity.GetUUID());

                    if (!l_Entity.HasComponent<MeshRendererComponent>() && ImGui::MenuItem("Mesh Renderer"))
                    {
                        m_Context.ComponentOp = [this, l_TargetUUID]()
                        {
                            Scene& l_OpScene = m_Engine.GetScene();
                            m_Context.History.Execute(std::make_unique<AddComponentCommand<MeshRendererComponent>>(l_OpScene, l_TargetUUID, "Mesh Renderer"));
                        };
                    }

                    if (!l_Entity.HasComponent<CameraComponent>() && ImGui::MenuItem("Camera"))
                    {
                        m_Context.ComponentOp = [this, l_TargetUUID]()
                        {
                            Scene& l_OpScene = m_Engine.GetScene();
                            m_Context.History.Execute(std::make_unique<AddComponentCommand<CameraComponent>>(l_OpScene, l_TargetUUID, "Camera"));
                        };
                    }

                    ImGui::EndPopup();
                }
            }
        }

        ImGui::End();
    }

    void InspectorPanel::DragTransformField(Scene& scene, uint64_t uuid, const char* label, TransformComponent& transform, glm::vec3& value, float speed)
    {
        ImGui::DragFloat3(label, &value.x, speed);

        if (ImGui::IsItemActivated())
        {
            m_TransformEditOld = transform;
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            m_Context.History.Execute(std::make_unique<SetTransformCommand>(scene, uuid, m_TransformEditOld, transform));
        }
    }
}