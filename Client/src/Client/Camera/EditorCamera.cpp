#include "EditorCamera.h"

#include "Engine/Application/Application.h"
#include "Engine/Input/Input.h"
#include "Engine/Platform/Window.h"

#include <algorithm>

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
}

void EditorCamera::OnUpdate(float deltaTime)
{
    const float l_MoveDistance = m_MoveSpeed * deltaTime;

    if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_W))
    {
        MoveForward(l_MoveDistance);
    }

    if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_S))
    {
        MoveBackward(l_MoveDistance);
    }

    if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_A))
    {
        MoveLeft(l_MoveDistance);
    }

    if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_D))
    {
        MoveRight(l_MoveDistance);
    }

    if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_Q))
    {
        MoveDown(l_MoveDistance);
    }

    if (Engine::Input::KeyDown(Engine::Code::KeyCode::TR_KEY_E))
    {
        MoveUp(l_MoveDistance);
    }

    const bool l_RightMouseDown = Engine::Input::MouseButtonDown(Engine::Code::MouseCode::TR_BUTTONRIGHT);
    if (l_RightMouseDown)
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

            const float l_YawOffset = l_MouseDelta.x * m_MouseSensitivity;
            const float l_PitchOffset = -l_MouseDelta.y * m_MouseSensitivity;

            const float l_CurrentPitch = GetPitch();
            const float l_ClampedPitch = std::clamp(l_CurrentPitch + l_PitchOffset, s_MinPitch, s_MaxPitch);
            const float l_PitchDelta = l_ClampedPitch - l_CurrentPitch;

            Rotate(l_YawOffset, l_PitchDelta);
        }
    }
    else
    {
        m_HasLastMousePosition = false;
    }

    const Engine::Input::Vector2 l_ScrollDelta = Engine::Input::MouseScrolled();
    if (l_ScrollDelta.y != 0.0f)
    {
        const float l_NewFov = std::clamp(GetFov() - (l_ScrollDelta.y * m_ScrollSensitivity), s_MinFov, s_MaxFov);
        SetFov(l_NewFov);
    }
}