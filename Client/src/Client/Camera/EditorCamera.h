#pragma once

#include "Engine/Renderer/Camera.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class EditorCamera : public Engine::Render::Camera
{
public:
    EditorCamera();

    void OnUpdate(float deltaTime);

private:
    glm::vec3 GetForwardDirection() const;
    glm::vec3 GetRightDirection() const;
    glm::vec3 GetUpDirection() const;

    glm::vec2 m_LastMousePosition = glm::vec2(0.0f, 0.0f);
    bool m_HasLastMousePosition = false;

    glm::vec3 m_FocalPoint = glm::vec3(0.0f, 0.0f, 0.0f);
    float m_Distance = 10.0f;

    float m_MoveSpeed = 5.0f;
    float m_ShiftSpeedMultiplier = 2.0f;
    float m_MouseSensitivity = 0.1f;
    float m_ScrollSensitivity = 1.0f;
    float m_SpeedScrollMultiplier = 0.5f;
    float m_MinMoveSpeed = 0.5f;
    float m_MaxMoveSpeed = 50.0f;
    float m_PanSpeed = 0.005f;
};