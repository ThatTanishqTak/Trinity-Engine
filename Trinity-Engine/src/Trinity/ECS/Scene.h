#pragma once

#include "Trinity/ECS/Entity.h"
#include "Trinity/ECS/Components.h"

#include <entt/entt.hpp>
#include <string>

namespace Trinity
{
    class Scene
    {
    public:
        Scene() = default;

        Entity CreateEntity(const std::string& name = "Entity");
        void DestroyEntity(Entity entity);

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