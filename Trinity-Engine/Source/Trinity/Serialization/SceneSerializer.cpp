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
#include <Trinity/Renderer/Meshes/MeshLibrary.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    static constexpr uint32_t k_SceneVersion = 1;

    static void SerializeEntity(YAML::Emitter& out, Scene& scene, entt::entity handle)
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
            out << YAML::Key << "MeshPath" << YAML::Value << l_MeshRenderer->MeshPath;
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
            SerializeEntity(l_Out, scene, l_Handle);
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

    bool SceneSerializer::Deserialize(Scene& scene, MeshLibrary& meshLibrary, const std::filesystem::path& path)
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

                if (YAML::Node l_TransformNode = l_EntityNode["Transform"])
                {
                    TransformComponent& l_Transform = l_Entity.GetComponent<TransformComponent>();
                    l_Transform.Translation = l_TransformNode["Translation"].as<glm::vec3>();
                    l_Transform.Rotation = l_TransformNode["Rotation"].as<glm::vec3>();
                    l_Transform.Scale = l_TransformNode["Scale"].as<glm::vec3>();
                }

                if (YAML::Node l_MeshNode = l_EntityNode["MeshRenderer"])
                {
                    std::string l_MeshPath = l_MeshNode["MeshPath"] ? l_MeshNode["MeshPath"].as<std::string>() : "";

                    MeshRendererComponent l_Component;
                    l_Component.MeshPath = l_MeshPath;
                    l_Component.MeshReference = l_MeshPath.empty() ? meshLibrary.GetCube() : meshLibrary.Load(l_MeshPath);
                    l_Entity.AddComponent<MeshRendererComponent>(l_Component);
                }

                if (YAML::Node l_CameraNode = l_EntityNode["Camera"])
                {
                    CameraComponent l_Component;
                    l_Component.Primary = l_CameraNode["Primary"] ? l_CameraNode["Primary"].as<bool>() : true;
                    l_Entity.AddComponent<CameraComponent>(l_Component);
                }

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
}