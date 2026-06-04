#pragma once

#include <string>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <Trinity/Core/UUID.h>

namespace Trinity
{
    class Entity;

    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        Entity CreateEntity(const std::string& name = "Entity");
        Entity CreateEntityWithUUID(UUID id, const std::string& name = "Entity");
        void DestroyEntity(Entity entity);

        void SetParent(Entity child, Entity parent);
        glm::mat4 GetWorldMatrix(entt::entity entity);

        Entity GetPrimaryCameraEntity();

        entt::registry& GetRegistry() { return m_Registry; }
        const entt::registry& GetRegistry() const { return m_Registry; }

    private:
        friend class Entity;

        entt::registry m_Registry;
    };
}