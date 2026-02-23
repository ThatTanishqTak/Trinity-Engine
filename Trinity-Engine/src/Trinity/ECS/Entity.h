#pragma once

#include "Trinity/Utilities/Log.h"

#include <entt/entt.hpp>
#include <cstdlib>

namespace Trinity
{
    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity handle, entt::registry* registry) : m_Handle(handle), m_Registry(registry)
        {

        }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            if (!m_Registry)
            {
                TR_CORE_CRITICAL("Entity::AddComponent called with null registry");
                std::abort();
            }

            if (HasComponent<T>())
            {
                TR_CORE_CRITICAL("Entity already has component");
                std::abort();
            }

            return m_Registry->emplace<T>(m_Handle, std::forward<Args>(args)...);
        }

        template<typename T>
        T& GetComponent()
        {
            if (!m_Registry)
            {
                TR_CORE_CRITICAL("Entity::GetComponent called with null registry");
                std::abort();
            }

            if (!HasComponent<T>())
            {
                TR_CORE_CRITICAL("Entity missing requested component");
                std::abort();
            }

            return m_Registry->get<T>(m_Handle);
        }

        template<typename T>
        const T& GetComponent() const
        {
            if (!m_Registry)
            {
                TR_CORE_CRITICAL("Entity::GetComponent (const) called with null registry");
                std::abort();
            }

            if (!HasComponent<T>())
            {
                TR_CORE_CRITICAL("Entity missing requested component (const)");
                std::abort();
            }

            return m_Registry->get<T>(m_Handle);
        }

        template<typename T>
        bool HasComponent() const
        {
            return m_Registry && m_Registry->all_of<T>(m_Handle);
        }

        template<typename T>
        void RemoveComponent()
        {
            if (!m_Registry)
            {
                TR_CORE_CRITICAL("Entity::RemoveComponent called with null registry");
                std::abort();
            }

            if (!HasComponent<T>())
            {
                TR_CORE_CRITICAL("Entity missing component to remove");
                std::abort();
            }

            m_Registry->remove<T>(m_Handle);
        }

        entt::entity GetHandle() const { return m_Handle; }
        explicit operator bool() const { return m_Handle != entt::null && m_Registry != nullptr; }

        bool operator==(const Entity& other) const { return m_Handle == other.m_Handle && m_Registry == other.m_Registry; }
        bool operator!=(const Entity& other) const { return !(*this == other); }

    private:
        entt::entity m_Handle{ entt::null };
        entt::registry* m_Registry = nullptr;
    };
}