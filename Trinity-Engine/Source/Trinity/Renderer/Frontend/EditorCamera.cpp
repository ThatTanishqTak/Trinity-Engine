#include <Trinity/Renderer/Frontend/EditorCamera.h>

#include <Trinity/Platform/Input.h>

namespace Trinity
{
    EditorCamera::EditorCamera()
    {
        RecalculateView();
    }

    EditorCamera::EditorCamera(float fovDegrees, float aspect, float nearClip, float farClip) : m_Fov(glm::radians(fovDegrees)), m_Aspect(aspect), m_Near(nearClip), m_Far(farClip)
    {
        RecalculateView();
    }

    void EditorCamera::SetViewportSize(float width, float height)
    {
        m_Aspect = (height > 0.0f) ? (width / height) : 1.0f;
        RecalculateView();
    }

    void EditorCamera::OnUpdate(const Input& input, float deltaTime, bool allowLook)
    {
        std::pair<float, float> l_MouseDelta = input.GetMouseDelta();

        if (!allowLook)
        {
            return;
        }

        m_Yaw += l_MouseDelta.first * m_MouseSensitivity;
        m_Pitch -= l_MouseDelta.second * m_MouseSensitivity;

        float l_LookStep = m_LookSpeed * deltaTime;
        if (input.IsKeyPressed(Key::Left))
        {
            m_Yaw -= l_LookStep;
        }

        if (input.IsKeyPressed(Key::Right))
        {
            m_Yaw += l_LookStep;
        }

        if (input.IsKeyPressed(Key::Up))
        {
            m_Pitch += l_LookStep;
        }

        if (input.IsKeyPressed(Key::Down))
        {
            m_Pitch -= l_LookStep;
        }

        m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);

        glm::vec3 l_Forward = GetForward();
        glm::vec3 l_Right = GetRight();
        glm::vec3 l_WorldUp(0.0f, 1.0f, 0.0f);

        float l_MoveStep = m_MoveSpeed * deltaTime;
        if (input.IsKeyPressed(Key::W))
        {
            m_Position += l_Forward * l_MoveStep;
        }

        if (input.IsKeyPressed(Key::S))
        {
            m_Position -= l_Forward * l_MoveStep;
        }

        if (input.IsKeyPressed(Key::D))
        {
            m_Position += l_Right * l_MoveStep;
        }

        if (input.IsKeyPressed(Key::A))
        {
            m_Position -= l_Right * l_MoveStep;
        }

        if (input.IsKeyPressed(Key::E))
        {
            m_Position += l_WorldUp * l_MoveStep;
        }

        if (input.IsKeyPressed(Key::Q))
        {
            m_Position -= l_WorldUp * l_MoveStep;
        }

        RecalculateView();
    }

    void EditorCamera::SetMoveSpeed(float speed)
    {
        m_MoveSpeed = glm::clamp(speed, 0.1f, 100.0f);
    }

    void EditorCamera::Focus(const glm::vec3& target, float distance)
    {
        m_Position = target - GetForward() * distance;
        RecalculateView();
    }

    void EditorCamera::RecalculateView()
    {
        m_Camera.LookAt(m_Position, m_Position + GetForward(), glm::vec3(0.0f, 1.0f, 0.0f));
        m_Camera.SetPerspective(m_Fov, m_Aspect, m_Near, m_Far);
    }

    glm::vec3 EditorCamera::GetForward() const
    {
        float l_YawRad = glm::radians(m_Yaw);
        float l_PitchRad = glm::radians(m_Pitch);

        glm::vec3 l_Forward;
        l_Forward.x = glm::cos(l_PitchRad) * glm::cos(l_YawRad);
        l_Forward.y = glm::sin(l_PitchRad);
        l_Forward.z = glm::cos(l_PitchRad) * glm::sin(l_YawRad);

        return glm::normalize(l_Forward);
    }

    glm::vec3 EditorCamera::GetRight() const
    {
        return glm::normalize(glm::cross(GetForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }
}