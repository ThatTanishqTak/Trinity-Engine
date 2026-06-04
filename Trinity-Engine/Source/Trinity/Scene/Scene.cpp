#include <Trinity/Scene/Scene.h>

#include <algorithm>

#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/IDComponent.h>
#include <Trinity/Scene/Components/NameComponent.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/HierarchyComponent.h>
#include <Trinity/Scene/Components/CameraComponent.h>

namespace Trinity
{
    Entity Scene::CreateEntity(const std::string& name)
    {
        return CreateEntityWithUUID(UUID(), name);
    }

    Entity Scene::CreateEntityWithUUID(UUID id, const std::string& name)
    {
        Entity l_Entity(m_Registry.create(), this);

        l_Entity.AddComponent<IDComponent>(IDComponent{ id });
        l_Entity.AddComponent<NameComponent>(NameComponent{ name.empty() ? "Entity" : name });
        l_Entity.AddComponent<TransformComponent>();

        return l_Entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        if (!entity.IsValid())
        {
            return;
        }

        entt::entity l_Handle = entity.GetHandle();

        if (auto* l_Hierarchy = m_Registry.try_get<HierarchyComponent>(l_Handle))
        {
            if (l_Hierarchy->Parent != entt::null)
            {
                if (auto* l_ParentHierarchy = m_Registry.try_get<HierarchyComponent>(l_Hierarchy->Parent))
                {
                    auto& l_Siblings = l_ParentHierarchy->Children;
                    l_Siblings.erase(std::remove(l_Siblings.begin(), l_Siblings.end(), l_Handle), l_Siblings.end());
                }
            }

            std::vector<entt::entity> l_Children = l_Hierarchy->Children;
            for (entt::entity l_Child : l_Children)
            {
                DestroyEntity(Entity(l_Child, this));
            }
        }

        m_Registry.destroy(l_Handle);
    }

    void Scene::SetParent(Entity child, Entity parent)
    {
        if (!child.IsValid())
        {
            return;
        }

        entt::entity l_Child = child.GetHandle();
        entt::entity l_NewParent = parent.IsValid() ? parent.GetHandle() : entt::null;

        m_Registry.get_or_emplace<HierarchyComponent>(l_Child);
        entt::entity l_OldParent = m_Registry.get<HierarchyComponent>(l_Child).Parent;

        if (l_OldParent != entt::null)
        {
            if (auto* l_OldParentHierarchy = m_Registry.try_get<HierarchyComponent>(l_OldParent))
            {
                auto& l_Kids = l_OldParentHierarchy->Children;
                l_Kids.erase(std::remove(l_Kids.begin(), l_Kids.end(), l_Child), l_Kids.end());
            }
        }

        if (l_NewParent != entt::null)
        {
            m_Registry.get_or_emplace<HierarchyComponent>(l_NewParent).Children.push_back(l_Child);
        }

        m_Registry.get<HierarchyComponent>(l_Child).Parent = l_NewParent;
    }

    glm::mat4 Scene::GetWorldMatrix(entt::entity entity)
    {
        glm::mat4 l_Local = glm::mat4(1.0f);

        if (auto* l_Transform = m_Registry.try_get<TransformComponent>(entity))
        {
            l_Local = l_Transform->GetLocalMatrix();
        }

        if (auto* l_Hierarchy = m_Registry.try_get<HierarchyComponent>(entity))
        {
            if (l_Hierarchy->Parent != entt::null)
            {
                return GetWorldMatrix(l_Hierarchy->Parent) * l_Local;
            }
        }

        return l_Local;
    }

    Entity Scene::GetPrimaryCameraEntity()
    {
        auto l_View = m_Registry.view<CameraComponent>();
        for (entt::entity l_Entity : l_View)
        {
            if (l_View.get<CameraComponent>(l_Entity).Primary)
            {
                return Entity(l_Entity, this);
            }
        }

        return Entity();
    }
}