#include "Forge/Panels/ViewportPanel.h"

#include "Trinity/Application/Application.h"
#include "Trinity/Platform/Window/Window.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Scene/Scene.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Scene/Components/MeshComponent.h"
#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Platform/Input/Desktop/DesktopInput.h"
#include "Trinity/Platform/Input/Desktop/DesktopInputCodes.h"

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
            auto& l_Registry = m_SelectionContext->ActiveScene->GetRegistry();
            auto l_View = l_Registry.view<Trinity::TransformComponent, Trinity::MeshComponent>();

            for (auto it_Entity : l_View)
            {
                auto& l_Transform = l_View.get<Trinity::TransformComponent>(it_Entity);
                auto& l_Mesh      = l_View.get<Trinity::MeshComponent>(it_Entity);

                if (!l_Mesh.MeshData) continue;

                Trinity::MeshDrawCommand l_Cmd{};
                l_Cmd.MeshRef = l_Mesh.MeshData;
                const glm::mat4 l_Matrix = l_Transform.GetLocalMatrix();
                std::memcpy(l_Cmd.Transform, glm::value_ptr(l_Matrix), sizeof(l_Cmd.Transform));
                m_SceneRenderer.SubmitMesh(l_Cmd);
            }
        }

        m_SceneRenderer.EndScene();
        m_SceneRenderer.Render();

        // Register the output texture if not already done (initial frame or after resize)
        if (m_ViewportTextureID == 0)
        {
            auto l_Output = m_SceneRenderer.GetFinalOutput();
            if (l_Output)
            {
                m_ViewportTextureID = Trinity::ImGuiLayer::Get().RegisterTexture(l_Output);
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

        const float l_Scroll = Trinity::DesktopInput::MouseScrolled().y;
        if (l_Scroll != 0.0f)
        {
            m_Camera.AdjustFOV(-l_Scroll * 2.0f);
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
        const bool l_InEditState  = (m_SelectionContext->State == EditorState::Edit);
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
            m_LastWidth  = l_Width;
            m_LastHeight = l_Height;
            m_PendingResizeWidth  = l_Width;
            m_PendingResizeHeight = l_Height;
            m_ResizeDirty = true;
        }

        if (m_ViewportTextureID != 0)
        {
            ImGui::Image(static_cast<ImTextureID>(m_ViewportTextureID), l_PanelSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
        }

        // Gap: in Play/Pause, use scene CameraComponent view
        if (m_SelectionContext->State != EditorState::Edit)
        {
            const ImVec2 l_TextPos = ImVec2(8.0f, ImGui::GetCursorStartPos().y + 36.0f);
            ImGui::GetWindowDrawList()->AddText(l_TextPos, IM_COL32(255, 200, 0, 180), "No Scene Camera — using editor camera fallback");
        }

        RenderGizmos();
        ImGui::End();
    }

    void ViewportPanel::RenderGizmos()
    {
        // Gizmos are suppressed during Play / Pause
        if (m_SelectionContext->State != EditorState::Edit)
        {
            return;
        }

        if (m_SelectionContext->SelectedEntity == entt::null)
        {
            return;
        }

        if (m_GizmoOperation == static_cast<ImGuizmo::OPERATION>(-1))
        {
            return;
        }

        if (m_SelectionContext->ActiveScene == nullptr)
        {
            return;
        }

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        const ImVec2 l_WindowPos = ImGui::GetWindowPos();
        const ImVec2 l_WindowSize = ImGui::GetWindowSize();
        ImGuizmo::SetRect(l_WindowPos.x, l_WindowPos.y, l_WindowSize.x, l_WindowSize.y);

        const glm::mat4& l_View = m_Camera.GetViewMatrix();
        glm::mat4 l_Projection = m_Camera.GetProjectionMatrix();
        l_Projection[1][1] *= -1.0f;  // Vulkan NDC Y-down correction — gizmo draw only

        auto& l_Transform = m_SelectionContext->ActiveScene->GetRegistry().get<Trinity::TransformComponent>(m_SelectionContext->SelectedEntity);
        glm::mat4 l_ModelMatrix = l_Transform.GetLocalMatrix();

        ImGuizmo::Manipulate(glm::value_ptr(l_View), glm::value_ptr(l_Projection), m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(l_ModelMatrix));

        if (ImGuizmo::IsUsing())
        {
            glm::vec3 l_Translation, l_Rotation, l_Scale;
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(l_ModelMatrix), glm::value_ptr(l_Translation), glm::value_ptr(l_Rotation), glm::value_ptr(l_Scale));

            l_Transform.Position = l_Translation;
            l_Transform.Rotation = glm::radians(l_Rotation);
            l_Transform.Scale = l_Scale;
        }
    }

    void ViewportPanel::RenderToolbar()
    {
        constexpr float l_BtnWidth = 64.0f;
        constexpr float l_BtnHeight = 22.0f;
        constexpr float l_Spacing = 4.0f;
        constexpr float l_TotalWidth = l_BtnWidth * 3.0f + l_Spacing * 2.0f;

        const EditorState l_State = m_SelectionContext->State;

        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - l_TotalWidth) * 0.5f);

        // Play / Resume
        const bool l_CanPlay = (l_State == EditorState::Edit || l_State == EditorState::Pause);
        if (!l_CanPlay)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button(l_State == EditorState::Pause ? "Resume" : "Play", ImVec2(l_BtnWidth, l_BtnHeight)))
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
        if (!l_CanPause) ImGui::BeginDisabled();
        if (ImGui::Button("Pause", ImVec2(l_BtnWidth, l_BtnHeight)))
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
        
        if (ImGui::Button("Stop", ImVec2(l_BtnWidth, l_BtnHeight)))
        {
            m_SelectionContext->State = EditorState::Edit;
        }

        if (!l_CanStop)
        {
            ImGui::EndDisabled();
        }
    }
}