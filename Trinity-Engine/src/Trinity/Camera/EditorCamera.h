#pragma once

#include "Trinity/Camera/Camera.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace Trinity
{
    class Event;
    class MouseScrolledEvent;

    class EditorCamera : public Camera
    {
    public:
        EditorCamera();

        void SetViewportSize(float width, float height) override;
        void SetPosition(const glm::vec3& position) override;
        void SetRotation(float yawDegrees, float pitchDegrees) override;
        void SetProjection(float fieldOfViewDegrees, float nearClip, float farClip) override;

        const glm::mat4& GetViewMatrix() const override;
        const glm::mat4& GetProjectionMatrix() const override;
        const glm::mat4& GetViewProjectionMatrix() const override;

        void OnUpdate(float deltaTime) override;
        void OnEvent(Event& event) override;

    private:
        bool OnMouseScrolledEvent(MouseScrolledEvent& event);

        void ApplyOrbit(const glm::vec2& mouseDelta);
        void ApplyPan(const glm::vec2& mouseDelta);
        void ApplyFreelook(const glm::vec2& mouseDelta, float deltaTime);
        void ApplyZoom(float scrollDelta);

        void SyncPositionFromOrbit();
        void SyncFocalPointFromPosition();
        glm::vec3 GetForwardDirection() const;
        glm::vec3 GetRightDirection() const;
        glm::vec3 GetUpDirection() const;
        void UpdateViewProjection();

    private:
        glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 5.0f);
        float m_YawDegrees = -90.0f;
        float m_PitchDegrees = 0.0f;

        float m_ViewportWidth = 1.0f;
        float m_ViewportHeight = 1.0f;

        float m_FieldOfViewDegrees = 45.0f;
        float m_NearClip = 0.01f;
        float m_FarClip = 1000.0f;

        glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
        glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
        glm::mat4 m_ViewProjectionMatrix = glm::mat4(1.0f);

        glm::vec3 m_FocalPoint = glm::vec3(0.0f);
        float m_Distance = 8.0f;

        float m_MoveSpeed = 7.5f;
        float m_MoveBoostScalar = 3.0f;
        float m_PanScalar = 0.0045f;
        float m_RotationScalar = 0.16f;
        float m_OrbitScalar = 0.24f;
        float m_ZoomScalar = 1.25f;

        float m_MinDistance = 0.1f;
        float m_MaxPitch = 89.0f;

        glm::vec2 m_LastMousePosition = glm::vec2(0.0f);
        bool m_HasLastMousePosition = false;
        bool m_WasFreelookActive = false;
        float m_EventScrollDelta = 0.0f;
    };
}