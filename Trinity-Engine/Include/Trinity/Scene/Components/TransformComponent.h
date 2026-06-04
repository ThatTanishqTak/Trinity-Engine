#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Trinity
{
    struct TransformComponent
    {
        glm::vec3 Translation{ 0.0f };
        glm::vec3 Rotation{ 0.0f };
        glm::vec3 Scale{ 1.0f };

        glm::mat4 GetLocalMatrix() const
        {
            return glm::translate(glm::mat4(1.0f), Translation) * glm::mat4_cast(glm::quat(Rotation)) * glm::scale(glm::mat4(1.0f), Scale);
        }
    };
}