#include "Forge/Panels/ViewportPanel.h"
#include "Forge/AssetPayload.h"

#include "Trinity/Application/Application.h"
#include "Trinity/Platform/Window/Window.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Scene/Scene.h"
#include "Trinity/Scene/Entity.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Scene/Components/MeshComponent.h"
#include "Trinity/Scene/Components/TextureComponent.h"
#include "Trinity/Asset/AssetRegistry.h"
#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Platform/Input/Desktop/DesktopInput.h"
#include "Trinity/Platform/Input/Desktop/DesktopInputCodes.h"

#include <filesystem>

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace Forge
{
    ViewportPanel::ViewportPanel(std::string name, SelectionContext* context) : Panel(std::move(name)), m_SelectionContext(context)
    {

    }

    void ViewportPanel::OnInitialize()
    {
        m_Camera = Trinity::EditorCamera(60.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
        m_SceneRenderer.Initialize(1280, 720);
    }

    void ViewportPanel::OnShutdown()
    {
        Trinity::Renderer::WaitIdle();

        if (m_ViewportTextureID != 0)
        {
            Trinity::ImGuiLayer::Get().UnregisterTexture(m_ViewportTextureID);
            m_ViewportTextureID = 0;
        }

        m_SceneRenderer.Shutdown();
    }

    void ViewportPanel::OnPreRender()
    {
        // Apply any resize that was detected last frame (before we render into the framebuffer)
        if (m_ResizeDirty)
        {
            m_ResizeDirty = false;

            if (m_ViewportTextureID != 0)
            {
                Trinity::Renderer::WaitIdle();
                Trinity::ImGuiLayer::Get().UnregisterTexture(m_ViewportTextureID);

                m_ViewportTextureID = 0;
            }

            m_Camera.SetAspectRatio(static_cast<float>(m_PendingResizeWidth) / static_cast<float>(m_PendingResizeHeight));
            m_SceneRenderer.OnResize(m_PendingResizeWidth, m_PendingResizeHeight);
        }

        // Submit the scene
        Trinity::SceneRenderData l_SceneData{};
        m_SceneRenderer.BeginScene(m_Camera, l_SceneData);

        if (m_SelectionContext && m_SelectionContext->ActiveScene)
        {
            auto& a_Registry = m_SelectionContext->ActiveScene->GetRegistry();
            auto a_View = a_Registry.view<Trinity::TransformComponent, Trinity::MeshComponent>();

            for (auto it_Entity : a_View)
            {
                auto& a_TransformComponent = a_View.get<Trinity::TransformComponent>(it_Entity);
                auto& a_MeshComponent = a_View.get<Trinity::MeshComponent>(it_Entity);

                if (!a_MeshComponent.MeshData)
                {
                    continue;
                }

                Trinity::MeshDrawCommand l_DrawCommand{};
                l_DrawCommand.MeshRef = a_MeshComponent.MeshData;

                if (a_Registry.all_of<Trinity::TextureComponent>(it_Entity))
                {
                    auto& a_TextureComponent = a_Registry.get<Trinity::TextureComponent>(it_Entity);
                    l_DrawCommand.AlbedoTexture = a_TextureComponent.TextureData;
                }

                if (!l_DrawCommand.AlbedoTexture)
                {
                    l_DrawCommand.AlbedoTexture = a_MeshComponent.MeshData->AlbedoTexture;
                }

                const glm::mat4 l_Matrix = a_TransformComponent.GetLocalMatrix();
                std::memcpy(l_DrawCommand.Transform, glm::value_ptr(l_Matrix), sizeof(l_DrawCommand.Transform));

                m_SceneRenderer.SubmitMesh(l_DrawCommand);
            }
        }

        m_SceneRenderer.EndScene();
        m_SceneRenderer.Render();

        if (m_ViewportTextureID == 0)
        {
            auto a_Output = m_SceneRenderer.GetFinalOutput();
            if (a_Output)
            {
                m_ViewportTextureID = Trinity::ImGuiLayer::Get().RegisterTexture(a_Output);
            }
        }
    }

    void ViewportPanel::OnUpdate(float deltaTime)
    {
        // Editor camera is suspended during Play / Pause
        if (m_SelectionContext->State != EditorState::Edit)
        {
            Trinity::Application::Get().GetWindow().SetCursorLocked(false);

            return;
        }

        if (!m_IsViewportHovered || !m_IsViewportFocused)
        {
            Trinity::Application::Get().GetWindow().SetCursorLocked(false);

            return;
        }

        if (Trinity::DesktopInput::MouseButtonDown(Trinity::Code::MouseCode::TR_BUTTON_RIGHT))
        {
            Trinity::Application::Get().GetWindow().SetCursorLocked(true);

            const float l_Speed = m_CameraSpeed * deltaTime;
            const Trinity::DesktopInput::Vector2 l_Delta = Trinity::DesktopInput::MouseDelta();

            m_Camera.Rotate(l_Delta.x * m_Sensitivity, l_Delta.y * m_Sensitivity);

            if (Trinity::DesktopInput::KeyDown(Trinity::Code::KeyCode::TR_KEY_W))
            {
                m_Camera.MoveForward(l_Speed);
            }

            if (Trinity::DesktopInput::KeyDown(Trinity::Code::KeyCode::TR_KEY_S))
            {
                m_Camera.MoveForward(-l_Speed);
            }

            if (Trinity::DesktopInput::KeyDown(Trinity::Code::KeyCode::TR_KEY_A))
            {
                m_Camera.MoveRight(-l_Speed);
            }

            if (Trinity::DesktopInput::KeyDown(Trinity::Code::KeyCode::TR_KEY_D))
            {
                m_Camera.MoveRight(l_Speed);
            }

            if (Trinity::DesktopInput::KeyDown(Trinity::Code::KeyCode::TR_KEY_E))
            {
                m_Camera.MoveUp(l_Speed);
            }

            if (Trinity::DesktopInput::KeyDown(Trinity::Code::KeyCode::TR_KEY_Q))
            {
                m_Camera.MoveUp(-l_Speed);
            }
        }
        else
        {
            Trinity::Application::Get().GetWindow().SetCursorLocked(false);
        }

        if (m_IsViewportHovered)
        {
            const float l_Scroll = Trinity::DesktopInput::MouseScrolled().y;
            if (l_Scroll != 0.0f)
            {
                m_Camera.MoveForward(l_Scroll * m_ZoomSpeed);
            }
        }
    }

    void ViewportPanel::OnRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(m_Name.c_str(), &m_Open);
        ImGui::PopStyleVar();

        m_IsViewportHovered = ImGui::IsWindowHovered();
        m_IsViewportFocused = ImGui::IsWindowFocused();

        RenderToolbar();

        const bool l_InCameraMode = Trinity::DesktopInput::MouseButtonDown(Trinity::Code::MouseCode::TR_BUTTON_RIGHT);
        const bool l_InEditState = (m_SelectionContext->State == EditorState::Edit);

        if (m_IsViewportFocused && l_InEditState && !l_InCameraMode && !ImGuizmo::IsUsing())
        {
            if (Trinity::DesktopInput::KeyPressed(Trinity::Code::KeyCode::TR_KEY_W))
            {
                m_GizmoOperation = ImGuizmo::TRANSLATE;
            }

            if (Trinity::DesktopInput::KeyPressed(Trinity::Code::KeyCode::TR_KEY_E))
            {
                m_GizmoOperation = ImGuizmo::ROTATE;
            }

            if (Trinity::DesktopInput::KeyPressed(Trinity::Code::KeyCode::TR_KEY_R))
            {
                m_GizmoOperation = ImGuizmo::SCALE;
            }

            if (Trinity::DesktopInput::KeyPressed(Trinity::Code::KeyCode::TR_KEY_Q))
            {
                m_GizmoOperation = static_cast<ImGuizmo::OPERATION>(-1);
            }
        }

        const ImVec2 l_PanelSize = ImGui::GetContentRegionAvail();
        const uint32_t l_Width = static_cast<uint32_t>(l_PanelSize.x);
        const uint32_t l_Height = static_cast<uint32_t>(l_PanelSize.y);

        if (l_Width > 0 && l_Height > 0 && (l_Width != m_LastWidth || l_Height != m_LastHeight))
        {
            m_LastWidth = l_Width;
            m_LastHeight = l_Height;
            m_PendingResizeWidth = l_Width;
            m_PendingResizeHeight = l_Height;

            m_ResizeDirty = true;
        }

        if (m_ViewportTextureID != 0)
        {
            ImGui::Image(static_cast<ImTextureID>(m_ViewportTextureID), l_PanelSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

            if (m_SelectionContext->State == EditorState::Edit && ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* l_Payload = ImGui::AcceptDragDropPayload(AssetPayloadID))
                {
                    const auto* a_Asset = static_cast<const AssetPayload*>(l_Payload->Data);
                    if (a_Asset->Type == Trinity::AssetType::Mesh)
                    {
                        HandleMeshDrop(a_Asset->Path);
                    }
                }

                ImGui::EndDragDropTarget();
            }
        }

        // Gap: in Play/Pause, use scene CameraComponent view
        if (m_SelectionContext->State != EditorState::Edit)
        {
            const ImVec2 l_TextPosition = ImVec2(8.0f, ImGui::GetCursorStartPos().y + 36.0f);
            ImGui::GetWindowDrawList()->AddText(l_TextPosition, IM_COL32(255, 200, 0, 180), "No Scene Camera — using editor camera fallback");
        }

        RenderGizmos();
        ImGui::End();
    }

    void ViewportPanel::RenderGizmos()
    {
        // Gizmos are suppressed during Play / Pause
        if (m_SelectionContext->State != EditorState::Edit || m_SelectionContext->SelectedEntity == entt::null || m_GizmoOperation == static_cast<ImGuizmo::OPERATION>(-1) || m_SelectionContext->ActiveScene == nullptr)
        {
            return;
        }

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        const ImVec2 l_WindowPosition = ImGui::GetWindowPos();
        const ImVec2 l_WindowSize = ImGui::GetWindowSize();
        ImGuizmo::SetRect(l_WindowPosition.x, l_WindowPosition.y, l_WindowSize.x, l_WindowSize.y);

        const glm::mat4& l_ViewMatrix = m_Camera.GetViewMatrix();
        glm::mat4 l_ProjectionMatrix = m_Camera.GetProjectionMatrix();
        l_ProjectionMatrix[1][1] *= -1.0f;  // Vulkan NDC Y-down correction — gizmo draw only

        auto& a_TransformComponent = m_SelectionContext->ActiveScene->GetRegistry().get<Trinity::TransformComponent>(m_SelectionContext->SelectedEntity);
        glm::mat4 l_ModelMatrix = a_TransformComponent.GetLocalMatrix();

        ImGuizmo::Manipulate(glm::value_ptr(l_ViewMatrix), glm::value_ptr(l_ProjectionMatrix), m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(l_ModelMatrix));

        if (ImGuizmo::IsUsing())
        {
            if (m_GizmoOperation == ImGuizmo::TRANSLATE)
            {
                a_TransformComponent.Position = glm::vec3(l_ModelMatrix[3]);
            }
            else if (m_GizmoOperation == ImGuizmo::ROTATE)
            {
                const glm::vec3 l_Colum0 = glm::normalize(glm::vec3(l_ModelMatrix[0]));
                const glm::vec3 l_Colum1 = glm::normalize(glm::vec3(l_ModelMatrix[1]));
                const glm::vec3 l_Colum2 = glm::normalize(glm::vec3(l_ModelMatrix[2]));

                const float l_RotationX = std::atan2f(-l_Colum2.y, l_Colum2.z);
                const float l_RotationY = std::atan2f(l_Colum2.x, std::sqrtf(l_Colum2.z * l_Colum2.z + l_Colum2.y * l_Colum2.y));
                const float l_RotationZ = std::atan2f(-l_Colum1.x, l_Colum0.x);

                a_TransformComponent.Rotation = glm::vec3(l_RotationX, l_RotationY, l_RotationZ);
            }
            else if (m_GizmoOperation == ImGuizmo::SCALE)
            {
                a_TransformComponent.Scale = glm::vec3(glm::length(glm::vec3(l_ModelMatrix[0])), glm::length(glm::vec3(l_ModelMatrix[1])), glm::length(glm::vec3(l_ModelMatrix[2])));
            }
        }
    }

    void ViewportPanel::HandleMeshDrop(const std::string& path)
    {
        if (!m_SelectionContext || !m_SelectionContext->ActiveScene)
        {
            return;
        }

        const Trinity::AssetHandle l_Handle = Trinity::AssetRegistry::Get().ImportAsset(path);
        if (l_Handle == Trinity::InvalidAsset)
        {
            return;
        }

        std::shared_ptr<Trinity::Mesh> l_MeshComponent = Trinity::AssetRegistry::Get().LoadMesh(l_Handle);
        if (!l_MeshComponent)
        {
            return;
        }

        auto& a_Registry = m_SelectionContext->ActiveScene->GetRegistry();
        const entt::entity l_Selected = m_SelectionContext->SelectedEntity;

        if (l_Selected != entt::null && a_Registry.all_of<Trinity::MeshComponent>(l_Selected))
        {
            auto& a_MeshComponent = a_Registry.get<Trinity::MeshComponent>(l_Selected);
            a_MeshComponent.MeshAssetUUID = l_Handle;
            a_MeshComponent.MeshData = l_MeshComponent;
        }
        else
        {
            const std::string l_Name = std::filesystem::path(path).stem().string();
            Trinity::Entity l_Entity = m_SelectionContext->ActiveScene->CreateEntity(l_Name);

            auto& a_MeshComponent = l_Entity.AddComponent<Trinity::MeshComponent>();
            a_MeshComponent.MeshAssetUUID = l_Handle;
            a_MeshComponent.MeshData = l_MeshComponent;

            m_SelectionContext->SelectedEntity = l_Entity.GetHandle();
        }
    }

    void ViewportPanel::RenderToolbar()
    {
        constexpr float l_ButtonWidth = 64.0f;
        constexpr float l_ButtonHeight = 22.0f;
        constexpr float l_Spacing = 4.0f;
        constexpr float l_TotalWidth = l_ButtonWidth * 3.0f + l_Spacing * 2.0f;

        const EditorState l_State = m_SelectionContext->State;

        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - l_TotalWidth) * 0.5f);

        // Play / Resume
        const bool l_CanPlay = (l_State == EditorState::Edit || l_State == EditorState::Pause);
        if (!l_CanPlay)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button(l_State == EditorState::Pause ? "Resume" : "Play", ImVec2(l_ButtonWidth, l_ButtonHeight)))
        {
            m_SelectionContext->State = EditorState::Play;
        }

        if (!l_CanPlay)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine(0.0f, l_Spacing);

        // Pause
        const bool l_CanPause = (l_State == EditorState::Play);
        if (!l_CanPause)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Pause", ImVec2(l_ButtonWidth, l_ButtonHeight)))
        {
            m_SelectionContext->State = EditorState::Pause;
        }

        if (!l_CanPause)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine(0.0f, l_Spacing);

        // Stop
        const bool l_CanStop = (l_State == EditorState::Play || l_State == EditorState::Pause);
        if (!l_CanStop)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Stop", ImVec2(l_ButtonWidth, l_ButtonHeight)))
        {
            m_SelectionContext->State = EditorState::Edit;
        }

        if (!l_CanStop)
        {
            ImGui::EndDisabled();
        }
    }
}