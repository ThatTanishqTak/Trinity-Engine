#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Trinity
{
    class Event;

    class Camera
    {
    public:
        virtual ~Camera() = default;

        virtual void SetViewportSize(float width, float height) = 0;
        virtual void SetPosition(const glm::vec3& position) = 0;
        virtual void SetRotation(float yawDegrees, float pitchDegrees) = 0;
        virtual void SetProjection(float fieldOfViewDegrees, float nearClip, float farClip) = 0;

        virtual const glm::mat4& GetViewMatrix() const = 0;
        virtual const glm::mat4& GetProjectionMatrix() const = 0;
        virtual const glm::mat4& GetViewProjectionMatrix() const = 0;

        virtual void OnUpdate(float deltaTime) = 0;
        virtual void OnEvent(Event& event) = 0;
    };
}