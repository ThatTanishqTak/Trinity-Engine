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
        entt::entity l_Handle = m_Registry.create();
        Entity l_Entity(l_Handle, &m_Registry);

        l_Entity.AddComponent<IDComponent>(GenerateID());
        l_Entity.AddComponent<TagComponent>(name);
        l_Entity.AddComponent<TransformComponent>();

        return l_Entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        if (!entity)
        {
            return;
        }

        m_Registry.destroy(entity.GetHandle());
    }

    Entity Scene::FindEntityByTag(const std::string& tag)
    {
        auto a_View = m_Registry.view<TagComponent>();
        for (auto it_Entity : a_View)
        {
            const TagComponent& l_Tag = a_View.get<TagComponent>(it_Entity);
            if (l_Tag.Tag == tag)
            {
                return Entity(it_Entity, &m_Registry);
            }
        }

        return Entity{};
    }
}