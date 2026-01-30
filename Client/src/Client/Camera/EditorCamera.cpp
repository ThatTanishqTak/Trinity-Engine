#include "EditorCamera.h"

#include "Engine/Application/Application.h"
#include "Engine/Input/Input.h"
#include "Engine/Platform/Window.h"

#include <algorithm>
#include <cmath>

namespace
{
    static constexpr float s_DefaultFov = 45.0f;
    static constexpr float s_DefaultNearPlane = 0.1f;
    static constexpr float s_DefaultFarPlane = 1000.0f;
    static constexpr float s_MinPitch = -89.0f;
    static constexpr float s_MaxPitch = 89.0f;
    static constexpr float s_MinFov = 30.0f;
    static constexpr float s_MaxFov = 90.0f;
}

EditorCamera::EditorCamera() : Engine::Render::Camera()
{
    const Engine::Window& l_Window = Engine::Application::Get().GetWindow();
    const float l_Width = static_cast<float>(l_Window.GetWidth());
    const float l_Height = static_cast<float>(l_Window.GetHeight());
    const float l_Aspect = (l_Height > 0.0f) ? (l_Width / l_Height) : 1.0f;

    SetFov(s_DefaultFov);
    SetAspect(l_Aspect);
    SetNear(s_DefaultNearPlane);
    SetFar(s_DefaultFarPlane);

    SetPosition(m_FocalPoint - (GetForwardDirection() * m_Distance));
}

void EditorCamera::OnUpdate(float deltaTime)
{
    const bool l_ShiftDown = Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_LEFT_SHIFT);
    const float l_MoveDistance = m_MoveSpeed * (l_ShiftDown ? m_ShiftSpeedMultiplier : 1.0f) * deltaTime;

    glm::vec3 l_MovementDelta(0.0f, 0.0f, 0.0f);
    const glm::vec3 l_Forward = GetForwardDirection();
    const glm::vec3 l_Right = GetRightDirection();
    const glm::vec3 l_Up = GetUpDirection();

    if (Engine::Input::MouseButtonDown(Engine::Code::MouseCode::TR_BUTTON_RIGHT))
    {

        if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_W))
        {
            l_MovementDelta += l_Forward * l_MoveDistance;
        }

        if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_S))
        {
            l_MovementDelta -= l_Forward * l_MoveDistance;
        }

        if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_A))
        {
            l_MovementDelta -= l_Right * l_MoveDistance;
        }

        if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_D))
        {
            l_MovementDelta += l_Right * l_MoveDistance;
        }

        if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_Q))
        {
            l_MovementDelta -= l_Up * l_MoveDistance;
        }

        if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_E))
        {
            l_MovementDelta += l_Up * l_MoveDistance;
        }
    }

    if (l_MovementDelta != glm::vec3(0.0f, 0.0f, 0.0f))
    {
        SetPosition(GetPosition() + l_MovementDelta);
        m_FocalPoint += l_MovementDelta;
    }

    const bool l_RightMouseDown = Engine::Input::MouseButtonDown(Engine::Code::MouseCode::TR_BUTTON_RIGHT);
    const bool l_MiddleMouseDown = Engine::Input::MouseButtonDown(Engine::Code::MouseCode::TR_BUTTON_MIDDLE);
    const bool l_PanActive = l_ShiftDown && (l_RightMouseDown || l_MiddleMouseDown);
    const bool l_OrbitActive = l_MiddleMouseDown && !l_ShiftDown;
    const bool l_MouseActionActive = l_RightMouseDown || l_MiddleMouseDown;

    if (l_MouseActionActive)
    {
        const Engine::Input::Vector2 l_CurrentMousePosition = Engine::Input::MousePosition();
        if (!m_HasLastMousePosition)
        {
            m_LastMousePosition = l_CurrentMousePosition;
            m_HasLastMousePosition = true;
        }
        else
        {
            const Engine::Input::Vector2 l_MouseDelta = l_CurrentMousePosition - m_LastMousePosition;
            m_LastMousePosition = l_CurrentMousePosition;

            if (l_OrbitActive)
            {
                const float l_YawOffset = l_MouseDelta.x * m_MouseSensitivity;
                const float l_PitchOffset = -l_MouseDelta.y * m_MouseSensitivity;

                const float l_CurrentPitch = GetPitch();
                const float l_ClampedPitch = std::clamp(l_CurrentPitch + l_PitchOffset, s_MinPitch, s_MaxPitch);
                const float l_PitchDelta = l_ClampedPitch - l_CurrentPitch;

                Rotate(l_YawOffset, l_PitchDelta);
                SetPosition(m_FocalPoint - (GetForwardDirection() * m_Distance));
            }
            else if (l_PanActive)
            {
                const glm::vec3 l_PanRight = GetRightDirection();
                const glm::vec3 l_PanUp = GetUpDirection();
                const float l_PanScale = m_PanSpeed * m_Distance;
                const glm::vec3 l_PanDelta = (-l_MouseDelta.x * l_PanScale * l_PanRight) + (l_MouseDelta.y * l_PanScale * l_PanUp);
                m_FocalPoint += l_PanDelta;
                SetPosition(GetPosition() + l_PanDelta);
            }
            else if (l_RightMouseDown)
            {
                const float l_YawOffset = l_MouseDelta.x * m_MouseSensitivity;
                const float l_PitchOffset = -l_MouseDelta.y * m_MouseSensitivity;

                const float l_CurrentPitch = GetPitch();
                const float l_ClampedPitch = std::clamp(l_CurrentPitch + l_PitchOffset, s_MinPitch, s_MaxPitch);
                const float l_PitchDelta = l_ClampedPitch - l_CurrentPitch;

                Rotate(l_YawOffset, l_PitchDelta);
                m_FocalPoint = GetPosition() + (GetForwardDirection() * m_Distance);
            }
        }
    }
    else
    {
        m_HasLastMousePosition = false;
    }

    const Engine::Input::Vector2 l_ScrollDelta = Engine::Input::MouseScrolled();
    if (l_ScrollDelta.y != 0.0f)
    {
        if (l_RightMouseDown)
        {
            m_MoveSpeed = std::clamp(m_MoveSpeed + (l_ScrollDelta.y * m_SpeedScrollMultiplier), m_MinMoveSpeed, m_MaxMoveSpeed);
        }
        else
        {
            const float l_NewFov = std::clamp(GetFov() - (l_ScrollDelta.y * m_ScrollSensitivity), s_MinFov, s_MaxFov);
            SetFov(l_NewFov);
        }
    }
}

glm::vec3 EditorCamera::GetForwardDirection() const
{
    glm::vec3 l_Forward(0.0f, 0.0f, 0.0f);
    l_Forward.x = std::cos(glm::radians(GetYaw())) * std::cos(glm::radians(GetPitch()));
    l_Forward.y = std::sin(glm::radians(GetPitch()));
    l_Forward.z = std::sin(glm::radians(GetYaw())) * std::cos(glm::radians(GetPitch()));
    return glm::normalize(l_Forward);
}

glm::vec3 EditorCamera::GetRightDirection() const
{
    return glm::normalize(glm::cross(GetForwardDirection(), GetWorldUp()));
}

glm::vec3 EditorCamera::GetUpDirection() const
{
    return glm::normalize(glm::cross(GetRightDirection(), GetForwardDirection()));
}