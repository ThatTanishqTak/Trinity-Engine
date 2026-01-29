#pragma once

#include <glm/glm.hpp>

namespace Engine
{
    namespace Render
    {
        class Camera
        {
        public:
            Camera();
            Camera(const glm::vec3& position, float yaw, float pitch, const glm::vec3& worldUp);
            virtual ~Camera() = default;

            virtual const glm::vec3& GetPosition() const;
            virtual void SetPosition(const glm::vec3& position);

            virtual float GetYaw() const;
            virtual void SetYaw(float yaw);

            virtual float GetPitch() const;
            virtual void SetPitch(float pitch);

            virtual const glm::vec3& GetWorldUp() const;
            virtual void SetWorldUp(const glm::vec3& worldUp);

            virtual float GetFov() const;
            virtual void SetFov(float fov);

            virtual float GetAspect() const;
            virtual void SetAspect(float aspect);

            virtual float GetNear() const;
            virtual void SetNear(float nearPlane);

            virtual float GetFar() const;
            virtual void SetFar(float farPlane);

            virtual glm::mat4 GetViewMatrix() const;
            virtual glm::mat4 GetProjectionMatrix() const;

            virtual void MoveForward(float distance);
            virtual void MoveBackward(float distance);
            virtual void MoveRight(float distance);
            virtual void MoveLeft(float distance);
            virtual void MoveUp(float distance);
            virtual void MoveDown(float distance);

            virtual void Rotate(float yawOffset, float pitchOffset);

        private:
            void UpdateCameraVectors();

            glm::vec3 m_Position;
            float m_Yaw;
            float m_Pitch;

            glm::vec3 m_WorldUp;
            glm::vec3 m_Front;
            glm::vec3 m_Right;
            glm::vec3 m_Up;

            float m_Fov;
            float m_Aspect;
            float m_Near;
            float m_Far;
        };
    }
}