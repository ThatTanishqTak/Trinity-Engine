#pragma once

#include "Trinity/ECS/Entity.h"
#include "Trinity/ECS/Components.h"

#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace Trinity
{
    class Scene
    {
    public:
        Scene() = default;

        Entity CreateEntity(const std::string& name = "Entity");
        void DestroyEntity(Entity entity);

        Entity FindEntityByTag(const std::string& tag);

        template<typename... T>
        std::vector<Entity> GetAllEntitiesWith()
        {
            std::vector<Entity> l_Entities;
            auto a_View = m_Registry.view<T...>();
            for (auto it_Entity : a_View)
            {
                l_Entities.emplace_back(it_Entity, &m_Registry);
            }

            return l_Entities;
        }

        entt::registry& GetRegistry() { return m_Registry; }
        const entt::registry& GetRegistry() const { return m_Registry; }

    private:
        EntityID GenerateID();

    private:
        entt::registry m_Registry;
        EntityID m_NextID = 1;

        friend class Entity;
    };
}