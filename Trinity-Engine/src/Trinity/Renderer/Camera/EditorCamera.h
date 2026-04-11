#pragma once

#include "Trinity/Renderer/Camera/Camera.h"

namespace Trinity
{
	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float fovDegrees, float aspectRatio, float nearClip, float farClip);

		void MoveForward(float distance);
		void MoveRight(float distance);
		void MoveUp(float distance);
		void Rotate(float yawDelta, float pitchDelta);
		void AdjustFOV(float delta);

		float GetYaw() const { return m_Yaw; }
		float GetPitch() const { return m_Pitch; }

	private:
		void UpdateDirectionVectors();

		float m_Yaw = -90.0f;
		float m_Pitch = 0.0f;
	};
}