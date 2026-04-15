#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace Trinity
{
    struct TransformComponent
    {
        glm::vec3 Position = glm::vec3(0.0f);
        glm::vec3 Rotation = glm::vec3(0.0f);  // Euler angles in radians
        glm::vec3 Scale    = glm::vec3(1.0f);

        uint64_t ParentUUID = 0;  // 0 = no parent (Phase 18.2)

        glm::mat4 GetLocalMatrix() const;
    };
}
