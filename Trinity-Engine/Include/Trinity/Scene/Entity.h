#pragma once

#include <utility>
#include <string>

#include <entt/entt.hpp>

#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/IDComponent.h>
#include <Trinity/Scene/Components/NameComponent.h>

namespace Trinity
{
    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene* scene) : m_Handle(handle), m_Scene(scene)
        {

        }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            return m_Scene->m_Registry.emplace<T>(m_Handle, std::forward<Args>(args)...);
        }

        template<typename T>
        T& GetComponent()
        {
            return m_Scene->m_Registry.get<T>(m_Handle);
        }

        template<typename T>
        const T& GetComponent() const
        {
            return m_Scene->m_Registry.get<T>(m_Handle);
        }

        template<typename T>
        T* TryGetComponent()
        {
            return m_Scene->m_Registry.try_get<T>(m_Handle);
        }

        template<typename T>
        bool HasComponent() const
        {
            return m_Scene->m_Registry.all_of<T>(m_Handle);
        }

        template<typename T>
        void RemoveComponent()
        {
            m_Scene->m_Registry.remove<T>(m_Handle);
        }

        UUID GetUUID() const { return GetComponent<IDComponent>().ID; }
        const std::string& GetName() const { return GetComponent<NameComponent>().Name; }

        entt::entity GetHandle() const { return m_Handle; }
        Scene* GetScene() const { return m_Scene; }

        bool IsValid() const { return m_Scene != nullptr && m_Handle != entt::null; }
        explicit operator bool() const { return IsValid(); }
        operator entt::entity() const { return m_Handle; }

        bool operator==(const Entity& other) const { return m_Handle == other.m_Handle && m_Scene == other.m_Scene; }
        bool operator!=(const Entity& other) const { return !(*this == other); }

    private:
        entt::entity m_Handle{ entt::null };
        Scene* m_Scene = nullptr;
    };
}