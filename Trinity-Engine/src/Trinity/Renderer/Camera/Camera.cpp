#include "Trinity/Renderer/Camera/Camera.h"

#include "Trinity/Utilities/Log.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Trinity
{
	Camera::Camera(float fovDegrees, float aspectRatio, float nearClip, float farClip) : m_FOV(fovDegrees), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
	{
		RecalculateViewMatrix();
		RecalculateProjectionMatrix();
	}

	void Camera::SetProjection(float fovDegrees, float aspectRatio, float nearClip, float farClip)
	{
		m_FOV = fovDegrees;
		m_AspectRatio = aspectRatio;
		m_NearClip = nearClip;
		m_FarClip = farClip;

		RecalculateProjectionMatrix();
	}

	void Camera::SetAspectRatio(float aspectRatio)
	{
		m_AspectRatio = aspectRatio;

		RecalculateProjectionMatrix();
	}

	void Camera::SetPosition(const glm::vec3& position)
	{
		m_Position = position;

		RecalculateViewMatrix();
	}

    void Camera::SetTransform(const glm::vec3& position, const glm::vec3& rotationRadians)
    {
        m_Position = position;

        glm::mat4 l_Rotation = glm::mat4(1.0f);
        l_Rotation = glm::rotate(l_Rotation, rotationRadians.x, glm::vec3(1.0f, 0.0f, 0.0f));
        l_Rotation = glm::rotate(l_Rotation, rotationRadians.y, glm::vec3(0.0f, 1.0f, 0.0f));
        l_Rotation = glm::rotate(l_Rotation, rotationRadians.z, glm::vec3(0.0f, 0.0f, 1.0f));

        m_Forward = glm::normalize(glm::vec3(l_Rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
        m_Right = glm::normalize(glm::cross(m_Forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        m_Up = glm::normalize(glm::cross(m_Right, m_Forward));

        RecalculateViewMatrix();
    }

	void Camera::RecalculateViewMatrix()
	{
		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
	}

	void Camera::RecalculateProjectionMatrix()
	{
		m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
		m_ProjectionMatrix[1][1] *= -1.0f;
	}
}