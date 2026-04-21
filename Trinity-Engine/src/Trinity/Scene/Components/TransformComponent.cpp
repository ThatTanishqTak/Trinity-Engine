#include "Trinity/Scene/Components/TransformComponent.h"

#include "Trinity/Scene/Scene.h"
#include "Trinity/Utilities/Log.h"

#include <glm/gtc/matrix_transform.hpp>

#include <entt/entt.hpp>

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

    glm::mat4 TransformComponent::GetWorldMatrix(const Scene& scene) const
    {
        constexpr int l_MaxDepth = 32;

        glm::mat4 l_World = GetLocalMatrix();

        uint64_t l_ParentUUID = ParentUUID;

        int l_Depth = 0;

        const auto& a_Registry = scene.GetRegistry();

        while (l_ParentUUID != 0)
        {
            if (++l_Depth > l_MaxDepth)
            {
                TR_CORE_WARN("TransformComponent::GetWorldMatrix: parent chain exceeded {} — assuming cycle, aborting walk", l_MaxDepth);
                break;
            }

            const entt::entity l_ParentHandle = scene.FindHandleByUUID(l_ParentUUID);
            if (l_ParentHandle == entt::null)
            {
                break;
            }

            const auto* a_ParentTransform = a_Registry.try_get<TransformComponent>(l_ParentHandle);
            if (!a_ParentTransform)
            {
                break;
            }

            l_World = a_ParentTransform->GetLocalMatrix() * l_World;
            l_ParentUUID = a_ParentTransform->ParentUUID;
        }

        return l_World;
    }
}