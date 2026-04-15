#pragma once

#include <entt/entt.hpp>

#include <string>

namespace Trinity
{
    class Entity;

    class Scene
    {
    public:
        explicit Scene(std::string name = "Untitled");
        ~Scene() = default;

        Entity CreateEntity(const std::string& tag = "Entity");
        void   DestroyEntity(Entity entity);

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        entt::registry& GetRegistry() { return m_Registry; }
        const entt::registry& GetRegistry() const { return m_Registry; }

    private:
        std::string    m_Name;
        entt::registry m_Registry;
    };
}
