#include "Trinity/Renderer/Camera/Camera.h"

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