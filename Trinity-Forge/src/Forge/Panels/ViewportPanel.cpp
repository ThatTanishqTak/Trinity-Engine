#include "Forge/Panels/ViewportPanel.h"

#include "Trinity/Application/Application.h"
#include "Trinity/Platform/Window/Window.h"
#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Platform/Input/Desktop/DesktopInput.h"
#include "Trinity/Platform/Input/Desktop/DesktopInputCodes.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

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
        if (m_ViewportTextureID != 0)
        {
            Trinity::ImGuiLayer::Get().UnregisterTexture(m_ViewportTextureID);
            m_ViewportTextureID = 0;
        }

        m_SceneRenderer.Shutdown();
    }

    void ViewportPanel::OnUpdate(float deltaTime)
    {
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

        const bool l_InCameraMode = Trinity::DesktopInput::MouseButtonDown(Trinity::Code::MouseCode::TR_BUTTON_RIGHT);
        if (m_IsViewportFocused && !l_InCameraMode && !ImGuizmo::IsUsing())
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

        if (l_Width != m_LastWidth || l_Height != m_LastHeight)
        {
            if (l_Width > 0 && l_Height > 0)
            {
                m_LastWidth  = l_Width;
                m_LastHeight = l_Height;

                if (m_ViewportTextureID != 0)
                {
                    Trinity::ImGuiLayer::Get().UnregisterTexture(m_ViewportTextureID);
                }

                m_Camera.SetAspectRatio(static_cast<float>(l_Width) / static_cast<float>(l_Height));
                m_SceneRenderer.OnResize(l_Width, l_Height);

                const auto l_Output = m_SceneRenderer.GetFinalOutput();
                if (l_Output)
                {
                    m_ViewportTextureID = Trinity::ImGuiLayer::Get().RegisterTexture(l_Output);
                }
            }
        }

        if (m_ViewportTextureID != 0)
        {
            ImGui::Image(static_cast<ImTextureID>(m_ViewportTextureID), l_PanelSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
        }

        RenderGizmos();
        ImGui::End();
    }

    void ViewportPanel::RenderGizmos()
    {
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

        // Gap: TransformComponent (Phase 18)
        // auto& l_Transform = m_SelectionContext->ActiveScene->GetRegistry().get<TransformComponent>(m_SelectionContext->SelectedEntity);
        // glm::mat4 l_ModelMatrix = l_Transform.GetMatrix();
        //
        // ImGuizmo::Manipulate(glm::value_ptr(l_View), glm::value_ptr(l_Projection),
        //     m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(l_ModelMatrix));
        //
        // if (ImGuizmo::IsUsing())
        // {
        //     glm::vec3 l_Translation, l_Rotation, l_Scale;
        //     ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(l_ModelMatrix),
        //         glm::value_ptr(l_Translation), glm::value_ptr(l_Rotation), glm::value_ptr(l_Scale));
        //     l_Transform.Position = l_Translation;
        //     l_Transform.Rotation = glm::radians(l_Rotation);
        //     l_Transform.Scale    = l_Scale;
        // }
    }
}
