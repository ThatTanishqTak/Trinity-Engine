#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Trinity
{
    struct TransformComponent
    {
        glm::vec3 Translation{ 0.0f };
        glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale{ 1.0f };

        // Euler accessors exist for the Inspector and gizmos only; everything else works with the quaternion or the matrix. Radians, same application order as the pre-quaternion glm::quat(euler) construction
        glm::vec3 GetEulerAngles() const { return glm::eulerAngles(Rotation); }
        void SetEulerAngles(const glm::vec3& euler) { Rotation = glm::quat(euler); }

        glm::mat4 GetLocalMatrix() const
        {
            return glm::translate(glm::mat4(1.0f), Translation) * glm::mat4_cast(Rotation) * glm::scale(glm::mat4(1.0f), Scale);
        }
    };
}