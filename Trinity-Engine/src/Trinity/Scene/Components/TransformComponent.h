#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <cstdint>

namespace Trinity
{
    struct TransformComponent
    {
        glm::vec3 Position = glm::vec3(0.0f);
        glm::vec3 Rotation = glm::vec3(0.0f);
        glm::vec3 Scale = glm::vec3(1.0f);

        uint64_t ParentUUID = 0;

        glm::mat4 GetLocalMatrix() const;
        glm::mat4 GetWorldMatrix(const entt::registry& registry) const;
    };
}