#pragma once

#include "Trinity/Scene/Scene.h"

namespace Trinity
{
    template<typename T, typename... Args>
    T& Entity::AddComponent(Args&&... args)
    {
        return m_Scene->GetRegistry().emplace<T>(m_Handle, std::forward<Args>(args)...);
    }

    template<typename T>
    T& Entity::GetComponent()
    {
        return m_Scene->GetRegistry().get<T>(m_Handle);
    }

    template<typename T>
    const T& Entity::GetComponent() const
    {
        return m_Scene->GetRegistry().get<T>(m_Handle);
    }

    template<typename T>
    bool Entity::HasComponent() const
    {
        return m_Scene->GetRegistry().all_of<T>(m_Handle);
    }

    template<typename T>
    void Entity::RemoveComponent()
    {
        m_Scene->GetRegistry().remove<T>(m_Handle);
    }
}
