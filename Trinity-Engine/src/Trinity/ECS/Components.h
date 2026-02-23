#pragma once

#include "Trinity/Geometry/Geometry.h"

#include <cstdint>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Trinity
{
    using EntityID = uint64_t;

    struct IDComponent
    {
        EntityID ID = 0;
    };

    struct TagComponent
    {
        std::string Tag;
    };

    struct TransformComponent
    {
        glm::vec3 Translation{ 0.0f };
        glm::vec3 Rotation{ 0.0f }; // radians
        glm::vec3 Scale{ 1.0f };

        glm::mat4 GetTransform() const
        {
            const glm::mat4 l_Translate = glm::translate(glm::mat4(1.0f), Translation);

            // Rotation stored as Euler radians -> quat -> mat4
            const glm::quat l_Quaternion = glm::quat(Rotation);
            const glm::mat4 l_Rotate = glm::toMat4(l_Quaternion);
            const glm::mat4 l_Scale = glm::scale(glm::mat4(1.0f), Scale);

            return l_Translate * l_Rotate * l_Scale;
        }
    };

    struct MeshRendererComponent
    {
        Geometry::PrimitiveType Primitive = Geometry::PrimitiveType::Cube;
        glm::vec4 Color{ 1.0f };

        MeshRendererComponent() = default;
        MeshRendererComponent(Geometry::PrimitiveType primitive, const glm::vec4& color) : Primitive(primitive), Color(color)
        {

        }
    };
}