#include "Forge/Panels/InspectorPanel.h"
#include "Forge/AssetPayload.h"

#include "Trinity/Scene/Scene.h"
#include "Trinity/Scene/Components/TagComponent.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Scene/Components/MeshComponent.h"
#include "Trinity/Scene/Components/TextureComponent.h"
#include "Trinity/Scene/Components/CameraComponent.h"
#include "Trinity/Scene/Components/DirectionalLightComponent.h"
#include "Trinity/Asset/AssetRegistry.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include <cctype>
#include <cstring>
#include <filesystem>

namespace Forge
{
    InspectorPanel::InspectorPanel(std::string name, SelectionContext* context)
        : Panel(std::move(name)), m_Context(context)
    {
    }

    void InspectorPanel::DrawVec3Control(const char* label, glm::vec3& values, float resetValue)
    {
        ImGui::PushID(label);

        ImGui::Text("%s", label);
        ImGui::SameLine(100.0f);

        const float l_BtnSize = ImGui::GetFrameHeight();
        const float l_Spacing = ImGui::GetStyle().ItemSpacing.x;
        const float l_DragWidth = (ImGui::GetContentRegionAvail().x - l_BtnSize * 3.0f - l_Spacing * 2.0f) / 3.0f;

        // X — red
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.10f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.80f, 0.10f, 0.10f, 1.0f));
        if (ImGui::Button("X", ImVec2(l_BtnSize, l_BtnSize))) values.x = resetValue;
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::SetNextItemWidth(l_DragWidth);
        ImGui::DragFloat("##X", &values.x, 0.1f);
        ImGui::SameLine(0.0f, l_Spacing);

        // Y — green
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.60f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.70f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.60f, 0.10f, 1.0f));
        if (ImGui::Button("Y", ImVec2(l_BtnSize, l_BtnSize))) values.y = resetValue;
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::SetNextItemWidth(l_DragWidth);
        ImGui::DragFloat("##Y", &values.y, 0.1f);
        ImGui::SameLine(0.0f, l_Spacing);

        // Z — blue
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.10f, 0.80f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.90f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.80f, 1.0f));
        if (ImGui::Button("Z", ImVec2(l_BtnSize, l_BtnSize))) values.z = resetValue;
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::SetNextItemWidth(l_DragWidth);
        ImGui::DragFloat("##Z", &values.z, 0.1f);

        ImGui::PopID();
    }

    void InspectorPanel::DrawAddComponentPopup(entt::registry& registry, entt::entity entity)
    {
        const float l_BtnWidth = 200.0f;
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - l_BtnWidth) * 0.5f + ImGui::GetCursorPosX());

        if (ImGui::Button("Add Component", ImVec2(l_BtnWidth, 0.0f)))
        {
            m_ComponentSearchBuffer[0] = '\0';
            ImGui::OpenPopup("##AddComponent");
        }

        ImGui::SetNextWindowSize(ImVec2(240.0f, 0.0f), ImGuiCond_Always);
        if (ImGui::BeginPopup("##AddComponent"))
        {
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##Search", m_ComponentSearchBuffer, sizeof(m_ComponentSearchBuffer));
            ImGui::Separator();

            // Case-insensitive substring match
            auto Matches = [&](const char* label) -> bool
            {
                if (m_ComponentSearchBuffer[0] == '\0') return true;
                const char* a = m_ComponentSearchBuffer;
                const char* b = label;
                // Simple case-insensitive contains
                for (; *b; ++b)
                {
                    const char* ai = a;
                    const char* bi = b;
                    while (*ai && *bi && std::tolower(static_cast<unsigned char>(*ai)) == std::tolower(static_cast<unsigned char>(*bi)))
                    {
                        ++ai; ++bi;
                    }
                    if (*ai == '\0') return true;
                }
                return false;
            };

            bool l_AnyVisible = false;

            if (!registry.all_of<Trinity::MeshComponent>(entity) && Matches("Mesh"))
            {
                if (ImGui::MenuItem("Mesh"))
                {
                    registry.emplace<Trinity::MeshComponent>(entity);
                    ImGui::CloseCurrentPopup();
                }
                l_AnyVisible = true;
            }

            if (!registry.all_of<Trinity::TextureComponent>(entity) && Matches("Texture"))
            {
                if (ImGui::MenuItem("Texture"))
                {
                    registry.emplace<Trinity::TextureComponent>(entity);
                    ImGui::CloseCurrentPopup();
                }
                l_AnyVisible = true;
            }

            if (!registry.all_of<Trinity::CameraComponent>(entity) && Matches("Camera"))
            {
                if (ImGui::MenuItem("Camera"))
                {
                    registry.emplace<Trinity::CameraComponent>(entity);
                    ImGui::CloseCurrentPopup();
                }
                l_AnyVisible = true;
            }

            if (!registry.all_of<Trinity::DirectionalLightComponent>(entity) && Matches("Directional Light"))
            {
                if (ImGui::MenuItem("Directional Light"))
                {
                    registry.emplace<Trinity::DirectionalLightComponent>(entity);
                    ImGui::CloseCurrentPopup();
                }
                l_AnyVisible = true;
            }

            if (!l_AnyVisible)
            {
                ImGui::TextDisabled("No components found");
            }

            ImGui::EndPopup();
        }
    }

    void InspectorPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        if (m_Context->SelectedEntity == entt::null || m_Context->ActiveScene == nullptr)
        {
            ImGui::End();
            return;
        }

        entt::registry& l_Registry = m_Context->ActiveScene->GetRegistry();
        const entt::entity l_Entity = m_Context->SelectedEntity;

        const bool l_ReadOnly = (m_Context->State != EditorState::Edit);
        if (l_ReadOnly)
        {
            ImGui::BeginDisabled();
        }

        // Tag — always present, no remove option
        if (l_Registry.all_of<Trinity::TagComponent>(l_Entity))
        {
            auto& l_Tag = l_Registry.get<Trinity::TagComponent>(l_Entity);

            char l_Buffer[256];
            std::strncpy(l_Buffer, l_Tag.Tag.c_str(), sizeof(l_Buffer) - 1);
            l_Buffer[sizeof(l_Buffer) - 1] = '\0';

            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText("##Tag", l_Buffer, sizeof(l_Buffer)))
                l_Tag.Tag = l_Buffer;
        }

        ImGui::Spacing();

        // Transform
        DrawComponent<Trinity::TransformComponent>("Transform", l_Registry, l_Entity, [](Trinity::TransformComponent& t)
        {
            DrawVec3Control("Position", t.Position, 0.0f);

            glm::vec3 l_RotDeg = glm::degrees(t.Rotation);
            DrawVec3Control("Rotation", l_RotDeg, 0.0f);
            t.Rotation = glm::radians(l_RotDeg);

            DrawVec3Control("Scale", t.Scale, 1.0f);
        });

        ImGui::Spacing();

        // Mesh
        DrawComponent<Trinity::MeshComponent>("Mesh", l_Registry, l_Entity, [](Trinity::MeshComponent& m)
        {
            const float l_AvailWidth = ImGui::GetContentRegionAvail().x;
            const float l_BtnSize = ImGui::GetFrameHeight();
            const float l_Spacing = ImGui::GetStyle().ItemSpacing.x;
            const float l_SlotWidth = l_AvailWidth - l_BtnSize - l_Spacing;
            const float l_SlotHeight = l_BtnSize + 4.0f;

            const ImVec2 l_SlotPos = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##MeshSlot", ImVec2(l_SlotWidth, l_SlotHeight));

            ImDrawList* l_Draw = ImGui::GetWindowDrawList();

            const bool l_HasMesh = (m.MeshData != nullptr);

            const ImU32 l_BorderColor = l_HasMesh
                ? ImGui::GetColorU32(ImGuiCol_Border)
                : ImGui::GetColorU32(ImGuiCol_Border, 0.4f);
            l_Draw->AddRect(l_SlotPos, ImVec2(l_SlotPos.x + l_SlotWidth, l_SlotPos.y + l_SlotHeight), l_BorderColor, 3.0f, 0, 1.0f);

            constexpr float l_IconSize = 14.0f;
            constexpr float l_IconPad = 5.0f;
            const ImVec2 l_IconMin(l_SlotPos.x + l_IconPad, l_SlotPos.y + (l_SlotHeight - l_IconSize) * 0.5f);
            const ImVec2 l_IconMax(l_IconMin.x + l_IconSize, l_IconMin.y + l_IconSize);
            const float  l_TextX = l_IconMax.x + 6.0f;
            const float  l_TextY = l_SlotPos.y + (l_SlotHeight - ImGui::GetTextLineHeight()) * 0.5f;

            if (l_HasMesh)
            {
                l_Draw->AddRectFilled(l_IconMin, l_IconMax, IM_COL32(33, 150, 243, 255), 2.0f);

                std::string l_FileName = "mesh";
                const Trinity::AssetMetadata* l_Meta = Trinity::AssetRegistry::Get().GetMetadata(m.MeshAssetUUID);
                if (l_Meta)
                {
                    l_FileName = std::filesystem::path(l_Meta->SourcePath).stem().string();
                }

                l_Draw->AddText(ImVec2(l_TextX, l_TextY), ImGui::GetColorU32(ImGuiCol_Text), l_FileName.c_str());
            }
            else
            {
                l_Draw->AddRectFilled(l_IconMin, l_IconMax, IM_COL32(33, 150, 243, 60), 2.0f);
                l_Draw->AddText(ImVec2(l_TextX, l_TextY), ImGui::GetColorU32(ImGuiCol_TextDisabled), "Drop Mesh Here");
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* l_Payload = ImGui::AcceptDragDropPayload(AssetPayloadID))
                {
                    const auto* a_Asset = static_cast<const AssetPayload*>(l_Payload->Data);
                    if (a_Asset->Type == Trinity::AssetType::Mesh)
                    {
                        const Trinity::AssetHandle l_Handle = Trinity::AssetRegistry::Get().ImportAsset(a_Asset->Path);
                        if (l_Handle != Trinity::InvalidAsset)
                        {
                            auto l_Mesh = Trinity::AssetRegistry::Get().LoadMesh(l_Handle);
                            if (l_Mesh)
                            {
                                m.MeshAssetUUID = l_Handle;
                                m.MeshData = l_Mesh;
                            }
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::SameLine(0.0f, l_Spacing);

            if (!l_HasMesh)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("x", ImVec2(l_BtnSize, l_SlotHeight)))
            {
                m.MeshAssetUUID = Trinity::InvalidAsset;
                m.MeshData.reset();
            }

            if (!l_HasMesh)
            {
                ImGui::EndDisabled();
            }

            if (m.MeshData)
            {
                ImGui::Spacing();
                ImGui::Text("Vertices: %u   Indices: %u", m.MeshData->GetVertexCount(), m.MeshData->GetIndexCount());
            }
        });

        ImGui::Spacing();

        // Texture
        DrawComponent<Trinity::TextureComponent>("Texture", l_Registry, l_Entity, [](Trinity::TextureComponent& t)
        {
            const float l_AvailWidth = ImGui::GetContentRegionAvail().x;
            const float l_BtnSize = ImGui::GetFrameHeight();
            const float l_Spacing = ImGui::GetStyle().ItemSpacing.x;
            const float l_SlotWidth = l_AvailWidth - l_BtnSize - l_Spacing;
            const float l_SlotHeight = l_BtnSize + 4.0f;

            const ImVec2 l_SlotPos = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##TextureSlot", ImVec2(l_SlotWidth, l_SlotHeight));

            ImDrawList* l_Draw = ImGui::GetWindowDrawList();

            const bool l_HasTex = (t.TextureData != nullptr);

            const ImU32 l_BorderColor = l_HasTex
                ? ImGui::GetColorU32(ImGuiCol_Border)
                : ImGui::GetColorU32(ImGuiCol_Border, 0.4f);
            l_Draw->AddRect(l_SlotPos, ImVec2(l_SlotPos.x + l_SlotWidth, l_SlotPos.y + l_SlotHeight), l_BorderColor, 3.0f, 0, 1.0f);

            constexpr float l_IconSize = 14.0f;
            constexpr float l_IconPad = 5.0f;
            const ImVec2 l_IconMin(l_SlotPos.x + l_IconPad, l_SlotPos.y + (l_SlotHeight - l_IconSize) * 0.5f);
            const ImVec2 l_IconMax(l_IconMin.x + l_IconSize, l_IconMin.y + l_IconSize);
            const float  l_TextX = l_IconMax.x + 6.0f;
            const float  l_TextY = l_SlotPos.y + (l_SlotHeight - ImGui::GetTextLineHeight()) * 0.5f;

            if (l_HasTex)
            {
                l_Draw->AddRectFilled(l_IconMin, l_IconMax, IM_COL32(233, 30, 99, 255), 2.0f);

                std::string l_FileName = "texture";
                const Trinity::AssetMetadata* l_Meta = Trinity::AssetRegistry::Get().GetMetadata(t.TextureAssetUUID);
                if (l_Meta)
                    l_FileName = std::filesystem::path(l_Meta->SourcePath).stem().string();

                l_Draw->AddText(ImVec2(l_TextX, l_TextY), ImGui::GetColorU32(ImGuiCol_Text), l_FileName.c_str());
            }
            else
            {
                l_Draw->AddRectFilled(l_IconMin, l_IconMax, IM_COL32(233, 30, 99, 60), 2.0f);
                l_Draw->AddText(ImVec2(l_TextX, l_TextY), ImGui::GetColorU32(ImGuiCol_TextDisabled), "Drop Texture Here");
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* l_Payload = ImGui::AcceptDragDropPayload(AssetPayloadID))
                {
                    const auto* a_Asset = static_cast<const AssetPayload*>(l_Payload->Data);
                    if (a_Asset->Type == Trinity::AssetType::Texture)
                    {
                        const Trinity::AssetHandle l_Handle = Trinity::AssetRegistry::Get().ImportAsset(a_Asset->Path);
                        if (l_Handle != Trinity::InvalidAsset)
                        {
                            auto l_Tex = Trinity::AssetRegistry::Get().LoadTexture(l_Handle);
                            if (l_Tex)
                            {
                                t.TextureAssetUUID = l_Handle;
                                t.TextureData = l_Tex;
                            }
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::SameLine(0.0f, l_Spacing);

            if (!l_HasTex)
                ImGui::BeginDisabled();

            if (ImGui::Button("x##tex", ImVec2(l_BtnSize, l_SlotHeight)))
            {
                t.TextureAssetUUID = Trinity::InvalidAsset;
                t.TextureData.reset();
            }

            if (!l_HasTex)
                ImGui::EndDisabled();
        });

        ImGui::Spacing();

        // Camera
        DrawComponent<Trinity::CameraComponent>("Camera", l_Registry, l_Entity, [](Trinity::CameraComponent& c)
        {
            ImGui::Checkbox("Primary", &c.Primary);
            ImGui::DragFloat("FOV", &c.FOV, 0.5f, 1.0f, 179.0f, "%.1f deg");
            ImGui::DragFloat("Near Clip", &c.NearClip, 0.01f, 0.001f, 10.0f, "%.3f");
            ImGui::DragFloat("Far Clip", &c.FarClip, 1.0f, 1.0f, 100000.0f, "%.0f");
        });

        ImGui::Spacing();

        // Directional Light
        DrawComponent<Trinity::DirectionalLightComponent>("Directional Light", l_Registry, l_Entity, [](Trinity::DirectionalLightComponent& l)
        {
            ImGui::ColorEdit3("Color", glm::value_ptr(l.Color));
            ImGui::DragFloat("Intensity", &l.Intensity, 0.01f, 0.0f, 100.0f);
        });

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        DrawAddComponentPopup(l_Registry, l_Entity);

        if (l_ReadOnly)
        {
            ImGui::EndDisabled();
        }

        ImGui::End();
    }
}
