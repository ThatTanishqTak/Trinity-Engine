#include "Trinity/Scene/Scene.h"

#include "Trinity/Scene/Entity.h"
#include "Trinity/Scene/Components/UUIDComponent.h"
#include "Trinity/Scene/Components/TagComponent.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Utilities/Log.h"

#include <random>

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
            TR_CORE_WARN("Scene::CreateEntityWithUUID: UUID 0 is reserved, regenerating");
            uuid = GenerateUUID();
        }

        if (m_EntityByUUID.find(uuid) != m_EntityByUUID.end())
        {
            TR_CORE_ERROR("Scene::CreateEntityWithUUID: UUID {} already in scene — regenerating", uuid);
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
        if (m_Registry.all_of<UUIDComponent>(l_Handle))
        {
            const uint64_t l_UUID = m_Registry.get<UUIDComponent>(l_Handle).UUID;
            m_EntityByUUID.erase(l_UUID);
        }

        m_Registry.destroy(l_Handle);
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
}