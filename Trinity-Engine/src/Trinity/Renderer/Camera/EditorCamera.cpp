#include "Trinity/Renderer/Camera/EditorCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace Trinity
{
    EditorCamera::EditorCamera(float fovDegrees, float aspectRatio, float nearClip, float farClip) : Camera(fovDegrees, aspectRatio, nearClip, farClip)
    {
        UpdateDirectionVectors();
    }

    void EditorCamera::MoveForward(float distance)
    {
        m_Position += m_Forward * distance;
        RecalculateViewMatrix();
    }

    void EditorCamera::MoveRight(float distance)
    {
        m_Position += m_Right * distance;
        RecalculateViewMatrix();
    }

    void EditorCamera::MoveUp(float distance)
    {
        m_Position += glm::vec3(0.0f, 1.0f, 0.0f) * distance;
        RecalculateViewMatrix();
    }

    void EditorCamera::Rotate(float yawDelta, float pitchDelta)
    {
        m_Yaw += yawDelta;
        m_Pitch -= pitchDelta;
        m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);
        UpdateDirectionVectors();
    }

    void EditorCamera::AdjustFOV(float delta)
    {
        m_FOV = std::clamp(m_FOV + delta, 1.0f, 120.0f);
        RecalculateProjectionMatrix();
    }

    void EditorCamera::UpdateDirectionVectors()
    {
        const float l_YawRadians = glm::radians(m_Yaw);
        const float l_PitchRadians = glm::radians(m_Pitch);

        m_Forward = glm::normalize(glm::vec3(std::cos(l_YawRadians) * std::cos(l_PitchRadians), std::sin(l_PitchRadians), std::sin(l_YawRadians) * std::cos(l_PitchRadians)));

        m_Right = glm::normalize(glm::cross(m_Forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        m_Up = glm::normalize(glm::cross(m_Right, m_Forward));

        RecalculateViewMatrix();
    }
}