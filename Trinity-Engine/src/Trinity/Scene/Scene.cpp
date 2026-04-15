#include "Trinity/Scene/Scene.h"

#include "Trinity/Scene/Entity.h"
#include "Trinity/Scene/Components/UUIDComponent.h"
#include "Trinity/Scene/Components/TagComponent.h"
#include "Trinity/Scene/Components/TransformComponent.h"

#include <random>

namespace Trinity
{
    static uint64_t GenerateUUID()
    {
        static std::random_device s_RandomDevice;
        static std::mt19937_64 s_Engine(s_RandomDevice());
        static std::uniform_int_distribution<uint64_t> s_Distribution;
        return s_Distribution(s_Engine);
    }

    Scene::Scene(std::string name)
        : m_Name(std::move(name))
    {
    }

    Entity Scene::CreateEntity(const std::string& tag)
    {
        Entity l_Entity(m_Registry.create(), this);
        l_Entity.AddComponent<UUIDComponent>(GenerateUUID());
        l_Entity.AddComponent<TagComponent>(tag);
        l_Entity.AddComponent<TransformComponent>();
        return l_Entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        m_Registry.destroy(entity.GetHandle());
    }
}
