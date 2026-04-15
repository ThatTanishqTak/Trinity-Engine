#include "Trinity/Scene/Entity.h"

namespace Trinity
{
    Entity::Entity(entt::entity handle, Scene* scene)
        : m_Handle(handle), m_Scene(scene)
    {
    }
}
