#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Trinity
{
    class Camera
    {
    public:
        void SetPerspective(float fovRadians, float aspect, float near, float far)
        {
            m_Projection = glm::perspective(fovRadians, aspect, near, far);
            m_Near = near;
            m_Far = far;
        }

        void SetOrthographic(float size, float aspect, float near, float far)
        {
            float l_HalfHeight = size * 0.5f;
            float l_HalfWidth = l_HalfHeight * aspect;

            m_Projection = glm::ortho(-l_HalfWidth, l_HalfWidth, -l_HalfHeight, l_HalfHeight, near, far);
            m_Near = near;
            m_Far = far;
        }

        void LookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up)
        {
            m_Position = eye;
            m_View = glm::lookAt(eye, target, up);
        }

        const glm::mat4& GetView() const { return m_View; }
        const glm::mat4& GetProjection() const { return m_Projection; }
        glm::mat4 GetViewProjection() const { return m_Projection * m_View; }
        const glm::vec3& GetPosition() const { return m_Position; }
        float GetNear() const { return m_Near; }
        float GetFar() const { return m_Far; }

    private:
        glm::mat4 m_View{ 1.0f };
        glm::mat4 m_Projection{ 1.0f };
        glm::vec3 m_Position{ 0.0f };
        float m_Near = 0.1f;
        float m_Far = 1000.0f;
    };
}