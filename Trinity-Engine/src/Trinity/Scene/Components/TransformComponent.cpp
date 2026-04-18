#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Scene/Components/UUIDComponent.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Trinity
{
    glm::mat4 TransformComponent::GetLocalMatrix() const
    {
        glm::mat4 l_Matrix = glm::mat4(1.0f);

        l_Matrix = glm::translate(l_Matrix, Position);
        l_Matrix = glm::rotate(l_Matrix, Rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        l_Matrix = glm::rotate(l_Matrix, Rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        l_Matrix = glm::rotate(l_Matrix, Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        l_Matrix = glm::scale(l_Matrix, Scale);

        return l_Matrix;
    }

    glm::mat4 TransformComponent::GetWorldMatrix(const entt::registry& registry) const
    {
        glm::mat4 l_Local = GetLocalMatrix();

        if (ParentUUID == 0)
            return l_Local;

        auto l_View = registry.view<const UUIDComponent, const TransformComponent>();
        for (auto l_Entity : l_View)
        {
            if (l_View.get<const UUIDComponent>(l_Entity).UUID == ParentUUID)
                return l_View.get<const TransformComponent>(l_Entity).GetWorldMatrix(registry) * l_Local;
        }

        return l_Local;
    }
}