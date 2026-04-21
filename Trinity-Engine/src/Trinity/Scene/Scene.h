#pragma once

#include <entt/entt.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Trinity
{
    class Entity;

    class Scene
    {
    public:
        explicit Scene(std::string name = "Untitled");
        ~Scene() = default;

        Scene(Scene&&) noexcept = default;
        Scene& operator=(Scene&&) noexcept = default;

        Entity CreateEntity(const std::string& tag = "Entity");
        Entity CreateEntityWithUUID(uint64_t uuid, const std::string& tag = "Entity");
        void DestroyEntity(Entity entity);

        Entity FindEntityByUUID(uint64_t uuid);
        entt::entity FindHandleByUUID(uint64_t uuid) const;

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        entt::registry& GetRegistry() { return m_Registry; }
        const entt::registry& GetRegistry() const { return m_Registry; }

    private:
        std::string m_Name;
        entt::registry m_Registry;
        std::unordered_map<uint64_t, entt::entity> m_EntityByUUID;
    };
}