#pragma once

#include "Engine/Renderer/Camera.h"

#include <glm/vec2.hpp>

class EditorCamera : public Engine::Render::Camera
{
public:
    EditorCamera();

    void OnUpdate(float deltaTime);

private:
    glm::vec2 m_LastMousePosition = glm::vec2(0.0f, 0.0f);
    bool m_HasLastMousePosition = false;

    float m_MoveSpeed = 5.0f;
    float m_MouseSensitivity = 0.1f;
    float m_ScrollSensitivity = 1.0f;
};