#pragma once

#include <glm/glm.hpp>

namespace Trinity
{
	class Camera
	{
	public:
		Camera() = default;
		Camera(float fovDegrees, float aspectRatio, float nearClip, float farClip);
		virtual ~Camera() = default;

		void SetProjection(float fovDegrees, float aspectRatio, float nearClip, float farClip);
		void SetAspectRatio(float aspectRatio);
		void SetPosition(const glm::vec3& position);

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }

		const glm::vec3& GetPosition() const { return m_Position; }
		const glm::vec3& GetForward() const { return m_Forward; }
		const glm::vec3& GetRight() const { return m_Right; }
		const glm::vec3& GetUp() const { return m_Up; }

		float GetFOV() const { return m_FOV; }
		float GetAspectRatio() const { return m_AspectRatio; }
		float GetNearClip() const { return m_NearClip; }
		float GetFarClip() const { return m_FarClip; }

	protected:
		void RecalculateViewMatrix();
		void RecalculateProjectionMatrix();

		glm::vec3 m_Position = { 0.0f, 0.0f, 3.0f };
		glm::vec3 m_Forward = { 0.0f, 0.0f, -1.0f };
		glm::vec3 m_Up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 m_Right = { 1.0f, 0.0f, 0.0f };

		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);

		float m_FOV = 60.0f;
		float m_AspectRatio = 16.0f / 9.0f;
		float m_NearClip = 0.1f;
		float m_FarClip = 1000.0f;
	};
}