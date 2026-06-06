#include <Trinity/Serialization/SceneSerializer.h>

#include <fstream>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include <Trinity/Serialization/YAMLUtilities.h>

#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/IDComponent.h>
#include <Trinity/Scene/Components/NameComponent.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/HierarchyComponent.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Scene/Components/CameraComponent.h>
#include <Trinity/Assets/AssetDatabase.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    static constexpr uint32_t k_SceneVersion = 1;

    static void EmitEntityNode(YAML::Emitter& out, Scene& scene, entt::entity handle)
    {
        entt::registry& l_Registry = scene.GetRegistry();

        out << YAML::BeginMap;
        out << YAML::Key << "UUID" << YAML::Value << static_cast<uint64_t>(l_Registry.get<IDComponent>(handle).ID);
        out << YAML::Key << "Name" << YAML::Value << l_Registry.get<NameComponent>(handle).Name;

        const TransformComponent& l_Transform = l_Registry.get<TransformComponent>(handle);
        out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Translation" << YAML::Value << l_Transform.Translation;
        out << YAML::Key << "Rotation" << YAML::Value << l_Transform.Rotation;
        out << YAML::Key << "Scale" << YAML::Value << l_Transform.Scale;
        out << YAML::EndMap;

        if (const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(handle))
        {
            if (l_Hierarchy->Parent != entt::null)
            {
                out << YAML::Key << "Parent" << YAML::Value << static_cast<uint64_t>(l_Registry.get<IDComponent>(l_Hierarchy->Parent).ID);
            }
        }

        if (const MeshRendererComponent* l_MeshRenderer = l_Registry.try_get<MeshRendererComponent>(handle))
        {
            out << YAML::Key << "MeshRenderer" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "MeshAsset" << YAML::Value << static_cast<uint64_t>(l_MeshRenderer->MeshAsset);
            out << YAML::EndMap;
        }

        if (const CameraComponent* l_Camera = l_Registry.try_get<CameraComponent>(handle))
        {
            out << YAML::Key << "Camera" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Primary" << YAML::Value << l_Camera->Primary;
            out << YAML::EndMap;
        }

        out << YAML::EndMap;
    }

    static void ReadEntityComponents(Entity entity, AssetDatabase& assetDatabase, const YAML::Node& node)
    {
        if (YAML::Node l_TransformNode = node["Transform"])
        {
            TransformComponent& l_Transform = entity.GetComponent<TransformComponent>();
            l_Transform.Translation = l_TransformNode["Translation"].as<glm::vec3>();
            l_Transform.Rotation = l_TransformNode["Rotation"].as<glm::vec3>();
            l_Transform.Scale = l_TransformNode["Scale"].as<glm::vec3>();
        }

        if (YAML::Node l_MeshNode = node["MeshRenderer"])
        {
            MeshRendererComponent l_Component;
            if (l_MeshNode["MeshAsset"])
            {
                l_Component.MeshAsset = UUID(l_MeshNode["MeshAsset"].as<uint64_t>());
            }

            l_Component.MeshReference = assetDatabase.ResolveMesh(l_Component.MeshAsset);
            entity.AddComponent<MeshRendererComponent>(l_Component);
        }

        if (YAML::Node l_CameraNode = node["Camera"])
        {
            CameraComponent l_Component;
            l_Component.Primary = l_CameraNode["Primary"] ? l_CameraNode["Primary"].as<bool>() : true;
            entity.AddComponent<CameraComponent>(l_Component);
        }
    }

    static void CollectSubtree(Scene& scene, entt::entity root, std::vector<entt::entity>& out)
    {
        out.push_back(root);

        entt::registry& l_Registry = scene.GetRegistry();
        if (const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(root))
        {
            for (entt::entity l_Child : l_Hierarchy->Children)
            {
                CollectSubtree(scene, l_Child, out);
            }
        }
    }

    bool SceneSerializer::Serialize(Scene& scene, const std::filesystem::path& path, const std::string& sceneName)
    {
        YAML::Emitter l_Out;
        l_Out << YAML::BeginMap;
        l_Out << YAML::Key << "Version" << YAML::Value << k_SceneVersion;
        l_Out << YAML::Key << "Scene" << YAML::Value << sceneName;
        l_Out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        auto l_View = scene.GetRegistry().view<IDComponent>();
        for (entt::entity l_Handle : l_View)
        {
            EmitEntityNode(l_Out, scene, l_Handle);
        }

        l_Out << YAML::EndSeq;
        l_Out << YAML::EndMap;

        std::error_code l_DirectoryError;
        std::filesystem::create_directories(path.parent_path(), l_DirectoryError);

        std::ofstream l_Stream(path);
        if (!l_Stream.is_open())
        {
            TR_CORE_ERROR("SceneSerializer: cannot open '{}' for writing", path.string());

            return false;
        }

        l_Stream << l_Out.c_str();
        TR_CORE_INFO("SceneSerializer: saved scene to '{}'", path.string());

        return true;
    }

    bool SceneSerializer::Deserialize(Scene& scene, AssetDatabase& assetDatabase, const std::filesystem::path& path)
    {
        try
        {
            YAML::Node l_Root = YAML::LoadFile(path.string());

            if (!l_Root["Version"])
            {
                TR_CORE_ERROR("SceneSerializer: '{}' missing Version field", path.string());

                return false;
            }

            uint32_t l_Version = l_Root["Version"].as<uint32_t>();
            if (l_Version != k_SceneVersion)
            {
                TR_CORE_WARN("SceneSerializer: '{}' version {} differs from expected {}", path.string(), l_Version, k_SceneVersion);
            }

            YAML::Node l_Entities = l_Root["Entities"];
            if (!l_Entities)
            {
                TR_CORE_WARN("SceneSerializer: '{}' has no entities", path.string());

                return true;
            }

            std::unordered_map<uint64_t, Entity> l_EntityMap;
            std::vector<std::pair<uint64_t, uint64_t>> l_PendingParents;

            for (const YAML::Node& l_EntityNode : l_Entities)
            {
                if (!l_EntityNode["UUID"])
                {
                    TR_CORE_WARN("SceneSerializer: entity missing UUID, skipped");

                    continue;
                }

                uint64_t l_UUID = l_EntityNode["UUID"].as<uint64_t>();
                std::string l_Name = l_EntityNode["Name"] ? l_EntityNode["Name"].as<std::string>() : "Entity";

                Entity l_Entity = scene.CreateEntityWithUUID(UUID(l_UUID), l_Name);
                l_EntityMap[l_UUID] = l_Entity;

                ReadEntityComponents(l_Entity, assetDatabase, l_EntityNode);

                if (l_EntityNode["Parent"])
                {
                    l_PendingParents.emplace_back(l_UUID, l_EntityNode["Parent"].as<uint64_t>());
                }
            }

            for (const std::pair<uint64_t, uint64_t>& l_Link : l_PendingParents)
            {
                auto it_Child = l_EntityMap.find(l_Link.first);
                auto it_Parent = l_EntityMap.find(l_Link.second);
                if (it_Child != l_EntityMap.end() && it_Parent != l_EntityMap.end())
                {
                    scene.SetParent(it_Child->second, it_Parent->second);
                }
                else
                {
                    TR_CORE_WARN("SceneSerializer: dangling parent reference {} -> {}", l_Link.first, l_Link.second);
                }
            }

            TR_CORE_INFO("SceneSerializer: loaded {} entities from '{}'", l_EntityMap.size(), path.string());

            return true;
        }
        catch (const std::exception& a_Exception)
        {
            TR_CORE_ERROR("SceneSerializer: failed to load '{}' ({})", path.string(), a_Exception.what());

            return false;
        }
    }

    std::string SceneSerializer::SerializeEntity(Scene& scene, Entity entity)
    {
        std::vector<entt::entity> l_Handles;
        CollectSubtree(scene, entity.GetHandle(), l_Handles);

        YAML::Emitter l_Out;
        l_Out << YAML::BeginMap;
        l_Out << YAML::Key << "Version" << YAML::Value << k_SceneVersion;
        l_Out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        for (entt::entity l_Handle : l_Handles)
        {
            EmitEntityNode(l_Out, scene, l_Handle);
        }

        l_Out << YAML::EndSeq;
        l_Out << YAML::EndMap;

        return std::string(l_Out.c_str());
    }

    Entity SceneSerializer::DeserializeEntity(Scene& scene, AssetDatabase& assetDatabase, const std::string& data, bool preserveUUIDs)
    {
        try
        {
            YAML::Node l_Root = YAML::Load(data);
            YAML::Node l_Entities = l_Root["Entities"];
            if (!l_Entities)
            {
                return Entity();
            }

            entt::registry& l_Registry = scene.GetRegistry();

            std::unordered_map<uint64_t, Entity> l_EntityMap;
            auto l_ExistingView = l_Registry.view<IDComponent>();
            for (entt::entity l_Existing : l_ExistingView)
            {
                l_EntityMap[static_cast<uint64_t>(l_Registry.get<IDComponent>(l_Existing).ID)] = Entity(l_Existing, &scene);
            }

            std::unordered_map<uint64_t, uint64_t> l_Remap;
            std::vector<std::pair<uint64_t, uint64_t>> l_PendingParents;

            Entity l_RootEntity;
            bool l_First = true;

            for (const YAML::Node& l_EntityNode : l_Entities)
            {
                if (!l_EntityNode["UUID"])
                {
                    continue;
                }

                uint64_t l_OldUUID = l_EntityNode["UUID"].as<uint64_t>();
                std::string l_Name = l_EntityNode["Name"] ? l_EntityNode["Name"].as<std::string>() : "Entity";

                UUID l_NewID = preserveUUIDs ? UUID(l_OldUUID) : UUID();
                l_Remap[l_OldUUID] = static_cast<uint64_t>(l_NewID);

                Entity l_Entity = scene.CreateEntityWithUUID(l_NewID, l_Name);
                l_EntityMap[static_cast<uint64_t>(l_NewID)] = l_Entity;

                ReadEntityComponents(l_Entity, assetDatabase, l_EntityNode);

                if (l_EntityNode["Parent"])
                {
                    l_PendingParents.emplace_back(static_cast<uint64_t>(l_NewID), l_EntityNode["Parent"].as<uint64_t>());
                }

                if (l_First)
                {
                    l_RootEntity = l_Entity;
                    l_First = false;
                }
            }

            for (const std::pair<uint64_t, uint64_t>& l_Link : l_PendingParents)
            {
                uint64_t l_ParentResolved = l_Link.second;
                auto it_Remap = l_Remap.find(l_Link.second);
                if (it_Remap != l_Remap.end())
                {
                    l_ParentResolved = it_Remap->second;
                }

                auto it_Child = l_EntityMap.find(l_Link.first);
                auto it_Parent = l_EntityMap.find(l_ParentResolved);
                if (it_Child != l_EntityMap.end() && it_Parent != l_EntityMap.end())
                {
                    scene.SetParent(it_Child->second, it_Parent->second);
                }
            }

            return l_RootEntity;
        }
        catch (const std::exception& a_Exception)
        {
            TR_CORE_ERROR("SceneSerializer: DeserializeEntity failed ({})", a_Exception.what());

            return Entity();
        }
    }
}