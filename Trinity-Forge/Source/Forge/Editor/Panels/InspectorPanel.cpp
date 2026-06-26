#include <Forge/Editor/Panels/InspectorPanel.h>

#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

#include <imgui.h>
#include <glm/glm.hpp>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>
#include <Forge/Editor/Commands/EntityCommands.h>
#include <Forge/Editor/Commands/ComponentCommands.h>
#include <Forge/Editor/Commands/PropertyCommands.h>
#include <Forge/Editor/Commands/MeshCommands.h>
#include <Forge/Editor/Commands/MaterialCommands.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/NameComponent.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Scene/Components/CameraComponent.h>
#include <Trinity/Scene/Components/LightComponent.h>
#include <Trinity/Scene/Components/AudioSourceComponent.h>
#include <Trinity/Scene/Components/AudioListenerComponent.h>
#include <Trinity/Renderer/Meshes/Mesh.h>
#include <Trinity/Renderer/Materials/Material.h>
#include <Trinity/Renderer/Materials/MaterialParameter.h>
#include <Trinity/Audio/Frontend/AudioEngine.h>
#include <Trinity/Serialization/MaterialSerializer.h>
#include <Trinity/Assets/AssetDatabase.h>

namespace Trinity
{
    static void DrawMaterialTextureParameter(AssetDatabase& assetDatabase, Material& material, const char* label, const char* parameterName)
    {
        UUID l_Texture = UUID(0);
        if (const MaterialParameter* l_TextureParameter = material.FindParameter(parameterName))
        {
            l_Texture = l_TextureParameter->AsTexture();
        }

        const AssetMetadata* l_TextureMeta = static_cast<uint64_t>(l_Texture) != 0 ? assetDatabase.GetMetadata(l_Texture) : nullptr;
        std::string l_TextureLabel = l_TextureMeta != nullptr ? l_TextureMeta->SourcePath : "(none)";

        if (ImGui::BeginCombo(label, l_TextureLabel.c_str()))
        {
            if (ImGui::Selectable("(none)", static_cast<uint64_t>(l_Texture) == 0))
            {
                material.SetParameter(parameterName, MaterialParameter::MakeTexture(UUID(0)));
            }

            for (UUID it_Texture : assetDatabase.GetAssetsOfType(AssetType::Texture))
            {
                const AssetMetadata* l_Meta = assetDatabase.GetMetadata(it_Texture);
                if (l_Meta == nullptr)
                {
                    continue;
                }

                bool l_Selected = static_cast<uint64_t>(it_Texture) == static_cast<uint64_t>(l_Texture);
                if (ImGui::Selectable(l_Meta->SourcePath.c_str(), l_Selected) && !l_Selected)
                {
                    material.SetParameter(parameterName, MaterialParameter::MakeTexture(it_Texture));
                }
            }

            ImGui::EndCombo();
        }
    }

    static void DrawMaterialEditor(Engine& engine, AssetDatabase& assetDatabase, UUID material)
    {
        const AssetMetadata* l_Metadata = assetDatabase.GetMetadata(material);
        if (l_Metadata == nullptr || l_Metadata->Type != AssetType::Material)
        {
            return;
        }

        std::shared_ptr<Material> l_Material = assetDatabase.ResolveMaterial(material);

        glm::vec4 l_Factor = glm::vec4(1.0f);
        if (const MaterialParameter* l_FactorParameter = l_Material->FindParameter(Material::BaseColorFactor))
        {
            l_Factor = l_FactorParameter->AsVec4();
        }

        if (ImGui::ColorEdit4("Base Color", &l_Factor.x))
        {
            l_Material->SetParameter(Material::BaseColorFactor, MaterialParameter::MakeVec4(l_Factor));
        }

        UUID l_Texture = UUID(0);
        if (const MaterialParameter* l_TextureParameter = l_Material->FindParameter(Material::BaseColorTexture))
        {
            l_Texture = l_TextureParameter->AsTexture();
        }

        const AssetMetadata* l_TextureMeta = static_cast<uint64_t>(l_Texture) != 0 ? assetDatabase.GetMetadata(l_Texture) : nullptr;
        std::string l_TextureLabel = l_TextureMeta != nullptr ? l_TextureMeta->SourcePath : "(none)";

        if (ImGui::BeginCombo("Base Color Texture", l_TextureLabel.c_str()))
        {
            if (ImGui::Selectable("(none)", static_cast<uint64_t>(l_Texture) == 0))
            {
                l_Material->SetParameter(Material::BaseColorTexture, MaterialParameter::MakeTexture(UUID(0)));
            }

            for (UUID it_Texture : assetDatabase.GetAssetsOfType(AssetType::Texture))
            {
                const AssetMetadata* l_Meta = assetDatabase.GetMetadata(it_Texture);
                if (l_Meta == nullptr)
                {
                    continue;
                }

                bool l_Selected = static_cast<uint64_t>(it_Texture) == static_cast<uint64_t>(l_Texture);
                if (ImGui::Selectable(l_Meta->SourcePath.c_str(), l_Selected) && !l_Selected)
                {
                    l_Material->SetParameter(Material::BaseColorTexture, MaterialParameter::MakeTexture(it_Texture));
                }
            }

            ImGui::EndCombo();
        }

        float l_Metallic = 0.0f;
        if (const MaterialParameter* l_Parameter = l_Material->FindParameter(Material::MetallicFactor))
        {
            l_Metallic = l_Parameter->AsFloat();
        }

        if (ImGui::SliderFloat("Metallic", &l_Metallic, 0.0f, 1.0f))
        {
            l_Material->SetParameter(Material::MetallicFactor, MaterialParameter::MakeFloat(l_Metallic));
        }

        float l_Roughness = 0.5f;
        if (const MaterialParameter* l_Parameter = l_Material->FindParameter(Material::RoughnessFactor))
        {
            l_Roughness = l_Parameter->AsFloat();
        }

        if (ImGui::SliderFloat("Roughness", &l_Roughness, 0.0f, 1.0f))
        {
            l_Material->SetParameter(Material::RoughnessFactor, MaterialParameter::MakeFloat(l_Roughness));
        }

        DrawMaterialTextureParameter(assetDatabase, *l_Material, "Metallic Roughness Texture", Material::MetallicRoughnessTexture);
        DrawMaterialTextureParameter(assetDatabase, *l_Material, "Normal Texture", Material::NormalTexture);

        float l_NormalScale = 1.0f;
        if (const MaterialParameter* l_Parameter = l_Material->FindParameter(Material::NormalScale))
        {
            l_NormalScale = l_Parameter->AsFloat();
        }

        if (ImGui::DragFloat("Normal Scale", &l_NormalScale, 0.01f, 0.0f, 4.0f))
        {
            l_Material->SetParameter(Material::NormalScale, MaterialParameter::MakeFloat(l_NormalScale));
        }

        glm::vec3 l_Emissive = glm::vec3(0.0f);
        if (const MaterialParameter* l_Parameter = l_Material->FindParameter(Material::EmissiveFactor))
        {
            l_Emissive = l_Parameter->AsVec3();
        }

        if (ImGui::ColorEdit3("Emissive", &l_Emissive.x))
        {
            l_Material->SetParameter(Material::EmissiveFactor, MaterialParameter::MakeVec3(l_Emissive));
        }

        float l_EmissiveStrength = 1.0f;
        if (const MaterialParameter* l_Parameter = l_Material->FindParameter(Material::EmissiveStrength))
        {
            l_EmissiveStrength = l_Parameter->AsFloat();
        }

        if (ImGui::DragFloat("Emissive Strength", &l_EmissiveStrength, 0.05f, 0.0f, 100.0f))
        {
            l_Material->SetParameter(Material::EmissiveStrength, MaterialParameter::MakeFloat(l_EmissiveStrength));
        }

        DrawMaterialTextureParameter(assetDatabase, *l_Material, "Emissive Texture", Material::EmissiveTexture);

        if (ImGui::SmallButton("Save##Material"))
        {
            std::filesystem::path l_Path = engine.GetPlatform().GetFileSystem().Resolve(BaseDirectory::Executable, l_Metadata->SourcePath);
            MaterialSerializer::SerializeMaterial(*l_Material, l_Path);
        }
    }

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
                    ImGui::SeparatorText(ICON_FA_UP_DOWN_LEFT_RIGHT "  Transform");
                    TransformComponent& l_Transform = l_Entity.GetComponent<TransformComponent>();
                    uint64_t l_TransformUUID = static_cast<uint64_t>(l_Entity.GetUUID());
                    DragTransformField(l_Scene, l_TransformUUID, "Translation", l_Transform, l_Transform.Translation, 0.1f);
                    DragTransformField(l_Scene, l_TransformUUID, "Rotation", l_Transform, l_Transform.Rotation, 0.01f);
                    DragTransformField(l_Scene, l_TransformUUID, "Scale", l_Transform, l_Transform.Scale, 0.1f);
                }

                if (l_Entity.HasComponent<MeshRendererComponent>())
                {
                    ImGui::SeparatorText(ICON_FA_CUBE "  Mesh Renderer");
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

                    if (l_MeshRenderer.MeshReference && l_MeshRenderer.MeshReference->IsValid())
                    {
                        ImGui::SeparatorText(ICON_FA_PAINTBRUSH "  Materials");

                        const std::vector<MaterialSlot>& l_MeshSlots = l_MeshRenderer.MeshReference->GetMaterialSlots();
                        size_t l_SlotCount = l_MeshSlots.empty() ? 1 : l_MeshSlots.size();

                        for (size_t l_SlotIndex = 0; l_SlotIndex < l_SlotCount; ++l_SlotIndex)
                        {
                            ImGui::PushID(static_cast<int>(l_SlotIndex));

                            std::string l_SlotName = l_SlotIndex < l_MeshSlots.size() && !l_MeshSlots[l_SlotIndex].Name.empty() ? l_MeshSlots[l_SlotIndex].Name : ("Slot " + std::to_string(l_SlotIndex));

                            uint64_t l_CurrentMaterial = l_SlotIndex < l_MeshRenderer.Materials.size() ? static_cast<uint64_t>(l_MeshRenderer.Materials[l_SlotIndex]) : 0;
                            std::string l_MaterialLabel;
                            if (l_CurrentMaterial == 0)
                            {
                                l_MaterialLabel = "(default)";
                            }
                            else
                            {
                                const AssetMetadata* l_Meta = l_Assets.GetMetadata(UUID(l_CurrentMaterial));
                                l_MaterialLabel = l_Meta != nullptr ? l_Meta->SourcePath : "(missing)";
                            }

                            if (ImGui::BeginCombo(l_SlotName.c_str(), l_MaterialLabel.c_str()))
                            {
                                if (ImGui::Selectable("(default)", l_CurrentMaterial == 0) && l_CurrentMaterial != 0)
                                {
                                    m_Context.History.Execute(std::make_unique<SetMaterialCommand>(l_Scene, l_MeshUUID, static_cast<uint32_t>(l_SlotIndex), UUID(0)));
                                }

                                for (UUID it_Material : l_Assets.GetAssetsOfType(AssetType::Material))
                                {
                                    const AssetMetadata* l_Meta = l_Assets.GetMetadata(it_Material);
                                    if (l_Meta == nullptr)
                                    {
                                        continue;
                                    }

                                    bool l_Selected = static_cast<uint64_t>(it_Material) == l_CurrentMaterial;
                                    if (ImGui::Selectable(l_Meta->SourcePath.c_str(), l_Selected) && !l_Selected)
                                    {
                                        m_Context.History.Execute(std::make_unique<SetMaterialCommand>(l_Scene, l_MeshUUID, static_cast<uint32_t>(l_SlotIndex), it_Material));
                                    }
                                }

                                for (UUID it_Material : l_Assets.GetAssetsOfType(AssetType::MaterialInstance))
                                {
                                    const AssetMetadata* l_Meta = l_Assets.GetMetadata(it_Material);
                                    if (l_Meta == nullptr)
                                    {
                                        continue;
                                    }

                                    bool l_Selected = static_cast<uint64_t>(it_Material) == l_CurrentMaterial;
                                    if (ImGui::Selectable(l_Meta->SourcePath.c_str(), l_Selected) && !l_Selected)
                                    {
                                        m_Context.History.Execute(std::make_unique<SetMaterialCommand>(l_Scene, l_MeshUUID, static_cast<uint32_t>(l_SlotIndex), it_Material));
                                    }
                                }

                                ImGui::EndCombo();
                            }

                            if (ImGui::BeginDragDropTarget())
                            {
                                if (const ImGuiPayload* l_Accepted = ImGui::AcceptDragDropPayload("TRINITY_ASSET_MATERIAL"))
                                {
                                    uint64_t l_Dropped = *static_cast<const uint64_t*>(l_Accepted->Data);
                                    m_Context.History.Execute(std::make_unique<SetMaterialCommand>(l_Scene, l_MeshUUID, static_cast<uint32_t>(l_SlotIndex), UUID(l_Dropped)));
                                }

                                ImGui::EndDragDropTarget();
                            }

                            if (l_CurrentMaterial != 0)
                            {
                                DrawMaterialEditor(m_Engine, l_Assets, UUID(l_CurrentMaterial));
                            }

                            ImGui::PopID();
                        }
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
                    ImGui::SeparatorText(ICON_FA_CAMERA "  Camera");
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

                if (l_Entity.HasComponent<LightComponent>())
                {
                    ImGui::SeparatorText(ICON_FA_LIGHTBULB "  Light");
                    LightComponent& l_Light = l_Entity.GetComponent<LightComponent>();

                    const char* l_TypeNames[] = { "Directional", "Point", "Spot" };
                    int l_TypeIndex = static_cast<int>(l_Light.Type);
                    if (ImGui::Combo("Type", &l_TypeIndex, l_TypeNames, IM_ARRAYSIZE(l_TypeNames)))
                    {
                        l_Light.Type = static_cast<LightType>(l_TypeIndex);
                    }

                    ImGui::ColorEdit3("Color", &l_Light.Color.x);
                    ImGui::DragFloat("Intensity", &l_Light.Intensity, 0.05f, 0.0f, 100.0f);

                    if (l_Light.Type != LightType::Directional)
                    {
                        ImGui::DragFloat("Range", &l_Light.Range, 0.1f, 0.0f, 1000.0f);
                    }

                    if (l_Light.Type == LightType::Spot)
                    {
                        ImGui::SliderAngle("Inner Cone", &l_Light.InnerConeAngle, 0.0f, 89.0f);
                        ImGui::SliderAngle("Outer Cone", &l_Light.OuterConeAngle, 0.0f, 90.0f);
                    }

                    if (ImGui::SmallButton("Remove##Light"))
                    {
                        uint64_t l_TargetUUID = static_cast<uint64_t>(l_Entity.GetUUID());
                        m_Context.ComponentOp = [this, l_TargetUUID]()
                            {
                                Scene& l_OpScene = m_Engine.GetScene();
                                m_Context.History.Execute(std::make_unique<RemoveComponentCommand<LightComponent>>(l_OpScene, l_TargetUUID, "Light"));
                            };
                    }
                }

                if (l_Entity.HasComponent<AudioSourceComponent>())
                {
                    ImGui::SeparatorText(ICON_FA_VOLUME_HIGH "  Audio Source");
                    AudioSourceComponent& l_Source = l_Entity.GetComponent<AudioSourceComponent>();
                    AssetDatabase& l_Assets = m_Engine.GetAssetDatabase();

                    uint64_t l_CurrentClip = static_cast<uint64_t>(l_Source.Clip);
                    std::string l_ClipLabel;
                    if (l_CurrentClip == 0)
                    {
                        l_ClipLabel = "(none)";
                    }
                    else
                    {
                        const AssetMetadata* l_Meta = l_Assets.GetMetadata(l_Source.Clip);
                        l_ClipLabel = l_Meta != nullptr ? l_Meta->SourcePath : "(missing)";
                    }

                    if (ImGui::BeginCombo("Clip", l_ClipLabel.c_str()))
                    {
                        if (ImGui::Selectable("(none)", l_CurrentClip == 0) && l_CurrentClip != 0)
                        {
                            l_Source.Clip = UUID(0);
                        }

                        for (UUID it_Clip : l_Assets.GetAssetsOfType(AssetType::Audio))
                        {
                            const AssetMetadata* l_Meta = l_Assets.GetMetadata(it_Clip);
                            if (l_Meta == nullptr)
                            {
                                continue;
                            }

                            bool l_Selected = static_cast<uint64_t>(it_Clip) == l_CurrentClip;
                            if (ImGui::Selectable(l_Meta->SourcePath.c_str(), l_Selected) && !l_Selected)
                            {
                                l_Source.Clip = it_Clip;
                            }
                        }

                        ImGui::EndCombo();
                    }

                    ImGui::DragFloat("Volume", &l_Source.Volume, 0.01f, 0.0f, 2.0f);
                    ImGui::DragFloat("Pitch", &l_Source.Pitch, 0.01f, 0.1f, 4.0f);
                    ImGui::Checkbox("Loop", &l_Source.Loop);
                    ImGui::Checkbox("Play On Start", &l_Source.PlayOnStart);
                    ImGui::Checkbox("Spatial", &l_Source.Spatial);

                    if (ImGui::SmallButton(ICON_FA_PLAY " Play##AudioSource"))
                    {
                        glm::vec3 l_WorldPosition = glm::vec3(l_Scene.GetWorldMatrix(l_Entity)[3]);
                        m_Engine.GetAudioEngine().PlaySource(l_Source, l_Assets, l_WorldPosition);
                    }

                    ImGui::SameLine();
                    if (ImGui::SmallButton(ICON_FA_STOP " Stop##AudioSource"))
                    {
                        m_Engine.GetAudioEngine().StopSource(l_Source);
                    }

                    if (ImGui::SmallButton("Remove##AudioSource"))
                    {
                        uint64_t l_TargetUUID = static_cast<uint64_t>(l_Entity.GetUUID());
                        m_Context.ComponentOp = [this, l_TargetUUID]()
                        {
                            Scene& l_OpScene = m_Engine.GetScene();
                            m_Context.History.Execute(std::make_unique<RemoveComponentCommand<AudioSourceComponent>>(l_OpScene, l_TargetUUID, "Audio Source"));
                        };
                    }
                }

                if (l_Entity.HasComponent<AudioListenerComponent>())
                {
                    ImGui::SeparatorText(ICON_FA_VOLUME_HIGH "  Audio Listener");
                    AudioListenerComponent& l_Listener = l_Entity.GetComponent<AudioListenerComponent>();
                    ImGui::Checkbox("Active", &l_Listener.Active);

                    if (ImGui::SmallButton("Remove##AudioListener"))
                    {
                        uint64_t l_TargetUUID = static_cast<uint64_t>(l_Entity.GetUUID());
                        m_Context.ComponentOp = [this, l_TargetUUID]()
                        {
                            Scene& l_OpScene = m_Engine.GetScene();
                            m_Context.History.Execute(std::make_unique<RemoveComponentCommand<AudioListenerComponent>>(l_OpScene, l_TargetUUID, "Audio Listener"));
                        };
                    }
                }

                ImGui::Separator();
                if (ImGui::Button(ICON_FA_PLUS "  Add Component"))
                {
                    ImGui::OpenPopup("AddComponentPopup");
                }

                if (ImGui::BeginPopup("AddComponentPopup"))
                {
                    uint64_t l_TargetUUID = static_cast<uint64_t>(l_Entity.GetUUID());

                    if (!l_Entity.HasComponent<MeshRendererComponent>() && ImGui::MenuItem(ICON_FA_CUBE "  Mesh Renderer"))
                    {
                        m_Context.ComponentOp = [this, l_TargetUUID]()
                        {
                            Scene& l_OpScene = m_Engine.GetScene();
                            m_Context.History.Execute(std::make_unique<AddComponentCommand<MeshRendererComponent>>(l_OpScene, l_TargetUUID, "Mesh Renderer"));
                        };
                    }

                    if (!l_Entity.HasComponent<CameraComponent>() && ImGui::MenuItem(ICON_FA_CAMERA "  Camera"))
                    {
                        m_Context.ComponentOp = [this, l_TargetUUID]()
                        {
                            Scene& l_OpScene = m_Engine.GetScene();
                            m_Context.History.Execute(std::make_unique<AddComponentCommand<CameraComponent>>(l_OpScene, l_TargetUUID, "Camera"));
                        };
                    }

                    if (!l_Entity.HasComponent<LightComponent>() && ImGui::MenuItem(ICON_FA_LIGHTBULB "  Light"))
                    {
                        m_Context.ComponentOp = [this, l_TargetUUID]()
                            {
                                Scene& l_OpScene = m_Engine.GetScene();
                                m_Context.History.Execute(std::make_unique<AddComponentCommand<LightComponent>>(l_OpScene, l_TargetUUID, "Light"));
                            };
                    }

                    if (!l_Entity.HasComponent<AudioSourceComponent>() && ImGui::MenuItem(ICON_FA_VOLUME_HIGH "  Audio Source"))
                    {
                        m_Context.ComponentOp = [this, l_TargetUUID]()
                        {
                            Scene& l_OpScene = m_Engine.GetScene();
                            m_Context.History.Execute(std::make_unique<AddComponentCommand<AudioSourceComponent>>(l_OpScene, l_TargetUUID, "Audio Source"));
                        };
                    }

                    if (!l_Entity.HasComponent<AudioListenerComponent>() && ImGui::MenuItem(ICON_FA_VOLUME_HIGH "  Audio Listener"))
                    {
                        m_Context.ComponentOp = [this, l_TargetUUID]()
                        {
                            Scene& l_OpScene = m_Engine.GetScene();
                            m_Context.History.Execute(std::make_unique<AddComponentCommand<AudioListenerComponent>>(l_OpScene, l_TargetUUID, "Audio Listener"));
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