#pragma once

#include <glm/glm.hpp>

#include <Trinity/Renderer/Frontend/Camera.h>

namespace Trinity
{
    class Input;

    class EditorCamera
    {
    public:
        EditorCamera();
        EditorCamera(float fovDegrees, float aspect, float nearClip, float farClip);

        void SetViewportSize(float width, float height);
        void OnUpdate(const Input& input, float deltaTime);

        const Camera& GetCamera() const { return m_Camera; }
        const glm::vec3& GetPosition() const { return m_Position; }

    private:
        void RecalculateView();
        glm::vec3 GetForward() const;
        glm::vec3 GetRight() const;

    private:
        Camera m_Camera;

        float m_Fov = glm::radians(45.0f);
        float m_Aspect = 1.0f;
        float m_Near = 0.1f;
        float m_Far = 100.0f;

        glm::vec3 m_Position{ 0.0f, 1.0f, 5.0f };
        float m_Yaw = -90.0f;
        float m_Pitch = 0.0f;

        float m_MoveSpeed = 5.0f;
        float m_LookSpeed = 90.0f;
    };
}