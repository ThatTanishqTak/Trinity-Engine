#pragma once

#include <glm/vec2.hpp>

#include <Trinity/Physics/Frontend/PhysicsMaterial.h>

namespace Trinity
{
    struct CircleCollider2DComponent
    {
        glm::vec2 Offset{ 0.0f };
        float Radius = 0.5f;
        bool IsTrigger = false;
        PhysicsMaterial Material;
    };
}