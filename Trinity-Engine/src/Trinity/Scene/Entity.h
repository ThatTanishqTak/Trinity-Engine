#pragma once

#include <entt/entt.hpp>

namespace Trinity
{
    class Scene;

    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene* scene);

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args);

        template<typename T>
        T& GetComponent();

        template<typename T>
        const T& GetComponent() const;

        template<typename T>
        bool HasComponent() const;

        template<typename T>
        void RemoveComponent();

        entt::entity GetHandle() const { return m_Handle; }

        operator entt::entity() const { return m_Handle; }
        operator bool()         const { return m_Handle != entt::null; }

        bool operator==(const Entity& other) const { return m_Handle == other.m_Handle && m_Scene == other.m_Scene; }
        bool operator!=(const Entity& other) const { return !(*this == other); }

    private:
        entt::entity m_Handle = entt::null;
        Scene*       m_Scene  = nullptr;
    };
}

#include "Trinity/Scene/Entity.inl"
