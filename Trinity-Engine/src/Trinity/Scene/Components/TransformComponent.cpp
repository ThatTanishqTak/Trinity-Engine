#include "Trinity/Scene/Components/TransformComponent.h"

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
}