#include "Trinity/Scene/Scene.h"

#include "Trinity/Scene/Entity.h"
#include "Trinity/Scene/Components/UUIDComponent.h"
#include "Trinity/Scene/Components/TagComponent.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Utilities/Log.h"

#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Trinity
{
    static uint64_t GenerateUUID()
    {
        static std::random_device s_RandomDevice;
        static std::mt19937_64 s_Engine(s_RandomDevice());
        static std::uniform_int_distribution<uint64_t> s_Distribution(1, UINT64_MAX);

        return s_Distribution(s_Engine);
    }

    Scene::Scene(std::string name) : m_Name(std::move(name))
    {

    }

    Entity Scene::CreateEntity(const std::string& tag)
    {
        return CreateEntityWithUUID(GenerateUUID(), tag);
    }

    Entity Scene::CreateEntityWithUUID(uint64_t uuid, const std::string& tag)
    {
        if (uuid == 0)
        {
            uuid = GenerateUUID();
        }

        if (m_EntityByUUID.find(uuid) != m_EntityByUUID.end())
        {
            uuid = GenerateUUID();
        }

        const entt::entity l_Handle = m_Registry.create();
        Entity l_Entity(l_Handle, this);

        l_Entity.AddComponent<UUIDComponent>(uuid);
        l_Entity.AddComponent<TagComponent>(tag);
        l_Entity.AddComponent<TransformComponent>();

        m_EntityByUUID.emplace(uuid, l_Handle);

        return l_Entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        const entt::entity l_Handle = entity.GetHandle();
        if (l_Handle == entt::null || !m_Registry.valid(l_Handle))
        {
            return;
        }

        std::vector<entt::entity> l_EntitiesToDestroy;
        l_EntitiesToDestroy.push_back(l_Handle);

        if (m_Registry.all_of<UUIDComponent, TransformComponent>(l_Handle))
        {
            std::unordered_map<uint64_t, std::vector<entt::entity>> l_ChildrenByParentUUID;

            auto a_View = m_Registry.view<UUIDComponent, TransformComponent>();
            for (auto it_Entity : a_View)
            {
                const auto& a_Transform = a_View.get<TransformComponent>(it_Entity);
                if (a_Transform.ParentUUID != 0)
                {
                    l_ChildrenByParentUUID[a_Transform.ParentUUID].push_back(it_Entity);
                }
            }

            std::vector<uint64_t> l_UUIDStack;
            std::unordered_set<uint64_t> l_VisitedUUIDs;

            const uint64_t l_RootUUID = m_Registry.get<UUIDComponent>(l_Handle).UUID;
            l_UUIDStack.push_back(l_RootUUID);
            l_VisitedUUIDs.insert(l_RootUUID);

            while (!l_UUIDStack.empty())
            {
                const uint64_t l_ParentUUID = l_UUIDStack.back();
                l_UUIDStack.pop_back();

                const auto a_ChildrenIt = l_ChildrenByParentUUID.find(l_ParentUUID);
                if (a_ChildrenIt == l_ChildrenByParentUUID.end())
                {
                    continue;
                }

                for (entt::entity it_Child : a_ChildrenIt->second)
                {
                    if (!m_Registry.valid(it_Child) || !m_Registry.all_of<UUIDComponent>(it_Child))
                    {
                        continue;
                    }

                    const uint64_t l_ChildUUID = m_Registry.get<UUIDComponent>(it_Child).UUID;
                    if (!l_VisitedUUIDs.insert(l_ChildUUID).second)
                    {
                        continue;
                    }

                    l_EntitiesToDestroy.push_back(it_Child);
                    l_UUIDStack.push_back(l_ChildUUID);
                }
            }
        }

        for (auto it_Entity = l_EntitiesToDestroy.rbegin(); it_Entity != l_EntitiesToDestroy.rend(); ++it_Entity)
        {
            if (!m_Registry.valid(*it_Entity))
            {
                continue;
            }

            if (m_Registry.all_of<UUIDComponent>(*it_Entity))
            {
                const uint64_t l_UUID = m_Registry.get<UUIDComponent>(*it_Entity).UUID;
                m_EntityByUUID.erase(l_UUID);
            }

            m_Registry.destroy(*it_Entity);
        }
    }

    Entity Scene::FindEntityByUUID(uint64_t uuid)
    {
        const auto a_Entity = m_EntityByUUID.find(uuid);
        if (a_Entity == m_EntityByUUID.end())
        {
            return Entity{};
        }

        return Entity(a_Entity->second, this);
    }

    entt::entity Scene::FindHandleByUUID(uint64_t uuid) const
    {
        const auto a_Entity = m_EntityByUUID.find(uuid);

        return a_Entity != m_EntityByUUID.end() ? a_Entity->second : entt::null;
    }

    void Scene::OnRuntimeStart()
    {
        TR_CORE_INFO("Scene runtime started: {}", m_Name);
    }

    void Scene::OnRuntimeUpdate(float deltaTime)
    {
        (void)deltaTime;
    }

    void Scene::OnRuntimeStop()
    {
        TR_CORE_INFO("Scene runtime stopped: {}", m_Name);
    }
}