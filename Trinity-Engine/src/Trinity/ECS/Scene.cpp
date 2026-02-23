#include "Trinity/ECS/Scene.h"
#include "Trinity/ECS/Components.h"

namespace Trinity
{
    EntityID Scene::GenerateID()
    {
        return m_NextID++;
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        entt::entity handle = m_Registry.create();
        Entity entity(handle, &m_Registry);

        entity.AddComponent<IDComponent>(GenerateID());
        entity.AddComponent<TagComponent>(name);
        entity.AddComponent<TransformComponent>();

        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        if (!entity)
        {
            return;
        }

        m_Registry.destroy(entity.GetHandle());
    }
}