#include <Forge/Editor/Commands/EntityCommands.h>

#include <Trinity/Serialization/SceneSerializer.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/IDComponent.h>
#include <Trinity/Scene/Components/NameComponent.h>
#include <Trinity/Scene/Components/HierarchyComponent.h>

namespace Trinity
{
    Entity FindEntityByUUID(Scene& scene, uint64_t uuid)
    {
        auto l_View = scene.GetRegistry().view<IDComponent>();
        for (entt::entity it_Entity : l_View)
        {
            if (static_cast<uint64_t>(l_View.get<IDComponent>(it_Entity).ID) == uuid)
            {
                return Entity(it_Entity, &scene);
            }
        }

        return Entity();
    }

    CreateEntityCommand::CreateEntityCommand(Scene& scene, const std::string& name) : m_Scene(scene), m_EntityName(name)
    {

    }

    void CreateEntityCommand::Execute()
    {
        if (!m_HasUUID)
        {
            Entity l_Entity = m_Scene.CreateEntity(m_EntityName);
            m_UUID = static_cast<uint64_t>(l_Entity.GetUUID());
            m_HasUUID = true;
        }
        else
        {
            m_Scene.CreateEntityWithUUID(UUID(m_UUID), m_EntityName);
        }
    }

    void CreateEntityCommand::Undo()
    {
        Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
        if (l_Entity.IsValid())
        {
            m_Scene.DestroyEntity(l_Entity);
        }
    }

    DeleteEntityCommand::DeleteEntityCommand(Scene& scene, AssetDatabase& assetDatabase, entt::entity handle) : m_Scene(scene), m_AssetDatabase(assetDatabase)
    {
        m_UUID = static_cast<uint64_t>(Entity(handle, &scene).GetUUID());
    }

    void DeleteEntityCommand::Execute()
    {
        Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
        if (!l_Entity.IsValid())
        {
            return;
        }

        if (m_Data.empty())
        {
            m_Data = SceneSerializer::SerializeEntity(m_Scene, l_Entity);
        }

        m_Scene.DestroyEntity(l_Entity);
    }

    void DeleteEntityCommand::Undo()
    {
        SceneSerializer::DeserializeEntity(m_Scene, m_AssetDatabase, m_Data, true);
    }

    DuplicateEntityCommand::DuplicateEntityCommand(Scene& scene, AssetDatabase& assetDatabase, entt::entity handle) : m_Scene(scene), m_AssetDatabase(assetDatabase)
    {
        m_SourceUUID = static_cast<uint64_t>(Entity(handle, &scene).GetUUID());
    }

    void DuplicateEntityCommand::Execute()
    {
        if (m_Data.empty())
        {
            Entity l_Source = FindEntityByUUID(m_Scene, m_SourceUUID);
            if (!l_Source.IsValid())
            {
                return;
            }

            std::string l_SourceData = SceneSerializer::SerializeEntity(m_Scene, l_Source);
            Entity l_Duplicate = SceneSerializer::DeserializeEntity(m_Scene, m_AssetDatabase, l_SourceData, false);
            if (!l_Duplicate.IsValid())
            {
                return;
            }

            m_DuplicateUUID = static_cast<uint64_t>(l_Duplicate.GetUUID());
            m_Data = SceneSerializer::SerializeEntity(m_Scene, l_Duplicate);
        }
        else
        {
            SceneSerializer::DeserializeEntity(m_Scene, m_AssetDatabase, m_Data, true);
        }
    }

    void DuplicateEntityCommand::Undo()
    {
        Entity l_Entity = FindEntityByUUID(m_Scene, m_DuplicateUUID);
        if (l_Entity.IsValid())
        {
            m_Scene.DestroyEntity(l_Entity);
        }
    }

    RenameEntityCommand::RenameEntityCommand(Scene& scene, uint64_t uuid, const std::string& oldName, const std::string& newName) : m_Scene(scene), m_UUID(uuid), m_OldName(oldName), m_NewName(newName)
    {

    }

    void RenameEntityCommand::Execute()
    {
        Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
        if (l_Entity.IsValid())
        {
            l_Entity.GetComponent<NameComponent>().Name = m_NewName;
        }
    }

    void RenameEntityCommand::Undo()
    {
        Entity l_Entity = FindEntityByUUID(m_Scene, m_UUID);
        if (l_Entity.IsValid())
        {
            l_Entity.GetComponent<NameComponent>().Name = m_OldName;
        }
    }

    static uint64_t ParentUUIDOf(Scene& scene, uint64_t childUUID)
    {
        Entity l_Child = FindEntityByUUID(scene, childUUID);
        if (!l_Child.IsValid())
        {
            return 0;
        }

        const HierarchyComponent* l_Hierarchy = scene.GetRegistry().try_get<HierarchyComponent>(l_Child.GetHandle());
        if (l_Hierarchy == nullptr || l_Hierarchy->Parent == entt::null)
        {
            return 0;
        }

        return static_cast<uint64_t>(scene.GetRegistry().get<IDComponent>(l_Hierarchy->Parent).ID);
    }

    SetParentCommand::SetParentCommand(Scene& scene, uint64_t childUUID, uint64_t newParentUUID) : m_Scene(scene), m_ChildUUID(childUUID), m_NewParentUUID(newParentUUID)
    {
        m_OldParentUUID = ParentUUIDOf(scene, childUUID);
    }

    void SetParentCommand::Execute()
    {
        Entity l_Child = FindEntityByUUID(m_Scene, m_ChildUUID);
        if (!l_Child.IsValid())
        {
            return;
        }

        Entity l_Parent = m_NewParentUUID != 0 ? FindEntityByUUID(m_Scene, m_NewParentUUID) : Entity();
        m_Scene.SetParent(l_Child, l_Parent);
    }

    void SetParentCommand::Undo()
    {
        Entity l_Child = FindEntityByUUID(m_Scene, m_ChildUUID);
        if (!l_Child.IsValid())
        {
            return;
        }

        Entity l_Parent = m_OldParentUUID != 0 ? FindEntityByUUID(m_Scene, m_OldParentUUID) : Entity();
        m_Scene.SetParent(l_Child, l_Parent);
    }
}