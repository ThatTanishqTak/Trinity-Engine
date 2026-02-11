#include "Trinity/Camera/EditorCamera.h"

#include "Trinity/Events/Event.h"
#include "Trinity/Events/MouseEvent.h"
#include "Trinity/Input/Input.h"
#include "Trinity/Application/Application.h"
#include "Trinity/Platform/Window.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace Trinity
{
    EditorCamera::EditorCamera()
    {
        SyncPositionFromOrbit();
        UpdateViewProjection();
    }

    void EditorCamera::SetViewportSize(float width, float height)
    {
        m_ViewportWidth = glm::max(width, 1.0f);
        m_ViewportHeight = glm::max(height, 1.0f);
        UpdateViewProjection();
    }

    void EditorCamera::SetPosition(const glm::vec3& position)
    {
        m_Position = position;
        SyncFocalPointFromPosition();
        UpdateViewProjection();
    }

    void EditorCamera::SetRotation(float yawDegrees, float pitchDegrees)
    {
        m_YawDegrees = yawDegrees;
        m_PitchDegrees = glm::clamp(pitchDegrees, -m_MaxPitch, m_MaxPitch);
        SyncPositionFromOrbit();
        UpdateViewProjection();
    }

    void EditorCamera::SetProjection(float fieldOfViewDegrees, float nearClip, float farClip)
    {
        m_FieldOfViewDegrees = fieldOfViewDegrees;
        m_NearClip = nearClip;
        m_FarClip = farClip;
        UpdateViewProjection();
    }

    const glm::mat4& EditorCamera::GetViewMatrix() const
    {
        return m_ViewMatrix;
    }

    const glm::mat4& EditorCamera::GetProjectionMatrix() const
    {
        return m_ProjectionMatrix;
    }

    const glm::mat4& EditorCamera::GetViewProjectionMatrix() const
    {
        return m_ViewProjectionMatrix;
    }

    void EditorCamera::OnUpdate(float deltaTime)
    {
        const Input::Vector2 l_MousePosition = Input::MousePosition();
        const glm::vec2 l_CurrentMousePosition = glm::vec2(l_MousePosition.x, l_MousePosition.y);

        glm::vec2 l_MouseDelta = glm::vec2(0.0f);
        if (m_HasLastMousePosition)
        {
            l_MouseDelta = l_CurrentMousePosition - m_LastMousePosition;
        }

        m_LastMousePosition = l_CurrentMousePosition;
        m_HasLastMousePosition = true;

        const bool l_AltDown = Input::KeyDown(Code::KeyCode::TR_KEY_LEFT_ALT) || Input::KeyDown(Code::KeyCode::TR_KEY_RIGHT_ALT);
        const bool l_RmbDown = Input::MouseButtonDown(Code::MouseCode::TR_BUTTON_RIGHT);
        const bool l_MmbDown = Input::MouseButtonDown(Code::MouseCode::TR_BUTTON_MIDDLE);
        const bool l_LmbDown = Input::MouseButtonDown(Code::MouseCode::TR_BUTTON_LEFT);

        Window& l_Window = Application::Get().GetWindow();

        if (l_RmbDown)
        {
            if (!m_WasFreelookActive)
            {
                l_Window.SetCursorVisible(false);
                l_Window.SetCursorLocked(true);
            }

            ApplyFreelook(l_MouseDelta, deltaTime);
            m_WasFreelookActive = true;
        }
        else
        {
            if (m_WasFreelookActive)
            {
                l_Window.SetCursorLocked(false);
                l_Window.SetCursorVisible(true);
            }

            m_WasFreelookActive = false;
        }

        if (l_MmbDown)
        {
            ApplyPan(l_MouseDelta);
        }
        else if (l_AltDown && l_LmbDown)
        {
            ApplyOrbit(l_MouseDelta);
        }

        const float l_ScrollDelta = Input::MouseScrolled().y + m_EventScrollDelta;
        m_EventScrollDelta = 0.0f;

        if (l_ScrollDelta != 0.0f)
        {
            ApplyZoom(l_ScrollDelta);
        }

        UpdateViewProjection();
    }

    void EditorCamera::OnEvent(Event& event)
    {
        EventDispatcher l_Dispatcher(event);
        l_Dispatcher.Dispatch<MouseScrolledEvent>(TR_BIND_EVENT_FN(OnMouseScrolledEvent));
    }

    bool EditorCamera::OnMouseScrolledEvent(MouseScrolledEvent& event)
    {
        m_EventScrollDelta += event.GetYOffset();

        return false;
    }

    void EditorCamera::ApplyOrbit(const glm::vec2& mouseDelta)
    {
        m_YawDegrees += mouseDelta.x * m_OrbitScalar;
        m_PitchDegrees -= mouseDelta.y * m_OrbitScalar;
        m_PitchDegrees = glm::clamp(m_PitchDegrees, -m_MaxPitch, m_MaxPitch);

        SyncPositionFromOrbit();
    }

    void EditorCamera::ApplyPan(const glm::vec2& mouseDelta)
    {
        const glm::vec3 l_RightDirection = GetRightDirection();
        const glm::vec3 l_UpDirection = GetUpDirection();
        const float l_PanAmount = m_PanScalar * glm::max(m_Distance, 1.0f);

        const glm::vec3 l_Offset = (-mouseDelta.x * l_RightDirection + -mouseDelta.y * l_UpDirection) * l_PanAmount;
        m_FocalPoint += l_Offset;
        m_Position += l_Offset;
    }

    void EditorCamera::ApplyFreelook(const glm::vec2& mouseDelta, float deltaTime)
    {
        m_YawDegrees += mouseDelta.x * m_RotationScalar;
        m_PitchDegrees += mouseDelta.y * m_RotationScalar;
        m_PitchDegrees = glm::clamp(m_PitchDegrees, -m_MaxPitch, m_MaxPitch);

        float l_MovementSpeed = m_MoveSpeed;
        if (Input::KeyDown(Code::KeyCode::TR_KEY_LEFT_SHIFT) || Input::KeyDown(Code::KeyCode::TR_KEY_RIGHT_SHIFT))
        {
            l_MovementSpeed *= m_MoveBoostScalar;
        }

        glm::vec3 l_MovementDirection = glm::vec3(0.0f);

        const glm::vec3 l_ForwardDirection = GetForwardDirection();
        const glm::vec3 l_RightDirection = GetRightDirection();
        const glm::vec3 l_WorldUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);

        if (Input::KeyDown(Code::KeyCode::TR_KEY_W))
        {
            l_MovementDirection += l_ForwardDirection;
        }
        if (Input::KeyDown(Code::KeyCode::TR_KEY_S))
        {
            l_MovementDirection -= l_ForwardDirection;
        }
        if (Input::KeyDown(Code::KeyCode::TR_KEY_D))
        {
            l_MovementDirection += l_RightDirection;
        }
        if (Input::KeyDown(Code::KeyCode::TR_KEY_A))
        {
            l_MovementDirection -= l_RightDirection;
        }
        if (Input::KeyDown(Code::KeyCode::TR_KEY_E))
        {
            l_MovementDirection += l_WorldUpDirection;
        }
        if (Input::KeyDown(Code::KeyCode::TR_KEY_Q))
        {
            l_MovementDirection -= l_WorldUpDirection;
        }

        if (glm::length2(l_MovementDirection) > 0.0f)
        {
            const glm::vec3 l_NormalizedDirection = glm::normalize(l_MovementDirection);
            m_Position += l_NormalizedDirection * l_MovementSpeed * deltaTime;
        }

        SyncFocalPointFromPosition();
    }

    void EditorCamera::ApplyZoom(float scrollDelta)
    {
        m_Distance -= scrollDelta * m_ZoomScalar;
        m_Distance = glm::max(m_Distance, m_MinDistance);
        SyncPositionFromOrbit();
    }

    void EditorCamera::SyncPositionFromOrbit()
    {
        const glm::vec3 l_ForwardDirection = GetForwardDirection();
        m_Position = m_FocalPoint - l_ForwardDirection * m_Distance;
    }

    void EditorCamera::SyncFocalPointFromPosition()
    {
        const glm::vec3 l_ForwardDirection = GetForwardDirection();
        m_FocalPoint = m_Position + l_ForwardDirection * m_Distance;
    }

    glm::vec3 EditorCamera::GetForwardDirection() const
    {
        const float l_YawRadians = glm::radians(m_YawDegrees);
        const float l_PitchRadians = glm::radians(m_PitchDegrees);

        glm::vec3 l_ForwardDirection = glm::vec3(0.0f);
        l_ForwardDirection.x = glm::cos(l_YawRadians) * glm::cos(l_PitchRadians);
        l_ForwardDirection.y = glm::sin(l_PitchRadians);
        l_ForwardDirection.z = glm::sin(l_YawRadians) * glm::cos(l_PitchRadians);

        return glm::normalize(l_ForwardDirection);
    }

    glm::vec3 EditorCamera::GetRightDirection() const
    {
        return glm::normalize(glm::cross(GetForwardDirection(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    glm::vec3 EditorCamera::GetUpDirection() const
    {
        return glm::normalize(glm::cross(GetRightDirection(), GetForwardDirection()));
    }

    void EditorCamera::UpdateViewProjection()
    {
        const glm::vec3 l_ForwardDirection = GetForwardDirection();
        m_ViewMatrix = glm::lookAt(m_Position, m_Position + l_ForwardDirection, glm::vec3(0.0f, 1.0f, 0.0f));

        const float l_AspectRatio = m_ViewportWidth / m_ViewportHeight;
        m_ProjectionMatrix = glm::perspective(glm::radians(m_FieldOfViewDegrees), l_AspectRatio, m_NearClip, m_FarClip);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }
}