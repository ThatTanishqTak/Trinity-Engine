#include "Engine/Renderer/Camera.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
    namespace Render
    {
        namespace
        {
            constexpr float l_DefaultFov = 45.0f;
            constexpr float l_DefaultAspect = 16.0f / 9.0f;
            constexpr float l_DefaultNear = 0.1f;
            constexpr float l_DefaultFar = 1000.0f;
            constexpr float l_PitchLimit = 89.0f;
        }

        Camera::Camera() : Camera(glm::vec3(0.0f, 0.0f, 0.0f), -90.0f, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f))
        {

        }

        Camera::Camera(const glm::vec3& position, float yaw, float pitch, const glm::vec3& worldUp): m_Position(position), m_Yaw(yaw), m_Pitch(pitch), m_WorldUp(worldUp),
            m_Front(0.0f, 0.0f, -1.0f), m_Right(1.0f, 0.0f, 0.0f), m_Up(0.0f, 1.0f, 0.0f), m_Fov(l_DefaultFov), m_Aspect(l_DefaultAspect), m_Near(l_DefaultNear), m_Far(l_DefaultFar)
        {
            SetPitch(m_Pitch);
            UpdateCameraVectors();
        }

        const glm::vec3& Camera::GetPosition() const
        {
            return m_Position;
        }

        void Camera::SetPosition(const glm::vec3& position)
        {
            m_Position = position;
        }

        float Camera::GetYaw() const
        {
            return m_Yaw;
        }

        void Camera::SetYaw(float yaw)
        {
            m_Yaw = yaw;
            UpdateCameraVectors();
        }

        float Camera::GetPitch() const
        {
            return m_Pitch;
        }

        void Camera::SetPitch(float pitch)
        {
            m_Pitch = std::clamp(pitch, -l_PitchLimit, l_PitchLimit);
            UpdateCameraVectors();
        }

        const glm::vec3& Camera::GetWorldUp() const
        {
            return m_WorldUp;
        }

        void Camera::SetWorldUp(const glm::vec3& worldUp)
        {
            m_WorldUp = worldUp;
            UpdateCameraVectors();
        }

        float Camera::GetFov() const
        {
            return m_Fov;
        }

        void Camera::SetFov(float fov)
        {
            m_Fov = fov;
        }

        float Camera::GetAspect() const
        {
            return m_Aspect;
        }

        void Camera::SetAspect(float aspect)
        {
            m_Aspect = aspect;
        }

        float Camera::GetNear() const
        {
            return m_Near;
        }

        void Camera::SetNear(float nearPlane)
        {
            m_Near = nearPlane;
        }

        float Camera::GetFar() const
        {
            return m_Far;
        }

        void Camera::SetFar(float farPlane)
        {
            m_Far = farPlane;
        }

        glm::mat4 Camera::GetViewMatrix() const
        {
            return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
        }

        glm::mat4 Camera::GetProjectionMatrix() const
        {
            return glm::perspective(glm::radians(m_Fov), m_Aspect, m_Near, m_Far);
        }

        void Camera::MoveForward(float distance)
        {
            m_Position += m_Front * distance;
        }

        void Camera::MoveBackward(float distance)
        {
            m_Position -= m_Front * distance;
        }

        void Camera::MoveRight(float distance)
        {
            m_Position += m_Right * distance;
        }

        void Camera::MoveLeft(float distance)
        {
            m_Position -= m_Right * distance;
        }

        void Camera::MoveUp(float distance)
        {
            m_Position += m_Up * distance;
        }

        void Camera::MoveDown(float distance)
        {
            m_Position -= m_Up * distance;
        }

        void Camera::Rotate(float yawOffset, float pitchOffset)
        {
            m_Yaw += yawOffset;
            m_Pitch = std::clamp(m_Pitch + pitchOffset, -l_PitchLimit, l_PitchLimit);
            UpdateCameraVectors();
        }

        void Camera::UpdateCameraVectors()
        {
            glm::vec3 l_Front;
            l_Front.x = std::cos(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch));
            l_Front.y = std::sin(glm::radians(m_Pitch));
            l_Front.z = std::sin(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch));
            m_Front = glm::normalize(l_Front);

            m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
            m_Up = glm::normalize(glm::cross(m_Right, m_Front));
        }
    }
}