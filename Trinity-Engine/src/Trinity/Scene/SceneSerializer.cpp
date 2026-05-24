#include "Trinity/Scene/SceneSerializer.h"

#include "Trinity/Geometry/Geometry.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Scene/Scene.h"
#include "Trinity/Scene/Entity.h"
#include "Trinity/Scene/Components/UUIDComponent.h"
#include "Trinity/Scene/Components/TagComponent.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Scene/Components/MeshComponent.h"
#include "Trinity/Scene/Components/CameraComponent.h"
#include "Trinity/Scene/Components/TextureComponent.h"
#include "Trinity/Asset/AssetRegistry.h"
#include "Trinity/Utilities/Log.h"

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

#include <fstream>

namespace YAML
{
    template<>
    struct convert<glm::vec3>
    {
        static Node encode(const glm::vec3& v)
        {
            Node l_Node;
            l_Node.push_back(v.x);
            l_Node.push_back(v.y);
            l_Node.push_back(v.z);
            l_Node.SetStyle(EmitterStyle::Flow);

            return l_Node;
        }

        static bool decode(const Node& node, glm::vec3& v)
        {
            if (!node.IsSequence() || node.size() != 3)
            {
                return false;
            }
            
            v.x = node[0].as<float>();
            v.y = node[1].as<float>();
            v.z = node[2].as<float>();
            
            return true;
        }
    };
}

namespace Trinity
{
    static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
    {
        out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;

        return out;
    }

    static void SerializeEntity(YAML::Emitter& out, Entity entity)
    {
        out << YAML::BeginMap;

        out << YAML::Key << "UUID" << YAML::Value << entity.GetComponent<UUIDComponent>().UUID;
        out << YAML::Key << "Tag" << YAML::Value << entity.GetComponent<TagComponent>().Tag;

        const auto& a_TransformComponent = entity.GetComponent<TransformComponent>();
        out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Position" << YAML::Value << a_TransformComponent.Position;
        out << YAML::Key << "Rotation" << YAML::Value << a_TransformComponent.Rotation;
        out << YAML::Key << "Scale" << YAML::Value << a_TransformComponent.Scale;
        out << YAML::Key << "Parent" << YAML::Value << a_TransformComponent.ParentUUID;
        out << YAML::EndMap;

        if (entity.HasComponent<MeshComponent>())
        {
            const auto& a_MeshComponent = entity.GetComponent<MeshComponent>();
            out << YAML::Key << "MeshComponent" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "MeshAssetUUID" << YAML::Value << a_MeshComponent.MeshAssetUUID;
            out << YAML::Key << "BuiltinType" << YAML::Value << static_cast<uint32_t>(a_MeshComponent.BuiltinType);

            const AssetMetadata* l_MetaData = AssetRegistry::Get().GetMetadata(a_MeshComponent.MeshAssetUUID);
            out << YAML::Key << "MeshSourcePath" << YAML::Value << (l_MetaData ? l_MetaData->SourcePath : std::string{});
            out << YAML::EndMap;
        }

        if (entity.HasComponent<CameraComponent>())
        {
            const auto& a_CameraComponent = entity.GetComponent<CameraComponent>();
            out << YAML::Key << "Camera" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "FOV" << YAML::Value << a_CameraComponent.FOV;
            out << YAML::Key << "NearClip" << YAML::Value << a_CameraComponent.NearClip;
            out << YAML::Key << "FarClip" << YAML::Value << a_CameraComponent.FarClip;
            out << YAML::Key << "Primary" << YAML::Value << a_CameraComponent.Primary;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<TextureComponent>())
        {
            const auto& a_TextureComponent = entity.GetComponent<TextureComponent>();

            out << YAML::Key << "TextureComponent" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "TextureAssetUUID" << YAML::Value << a_TextureComponent.TextureAssetUUID;

            const AssetMetadata* l_MetaData = AssetRegistry::Get().GetMetadata(a_TextureComponent.TextureAssetUUID);
            out << YAML::Key << "TextureSourcePath" << YAML::Value << (l_MetaData ? l_MetaData->SourcePath : std::string{});

            out << YAML::EndMap;
        }

        out << YAML::EndMap;
    }

    static std::string BuildYaml(Scene& scene)
    {
        YAML::Emitter l_Out;
        l_Out << YAML::BeginMap;
        l_Out << YAML::Key << "Scene" << YAML::Value << scene.GetName();
        l_Out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        scene.GetRegistry().view<UUIDComponent>().each([&](entt::entity handle, const UUIDComponent&)
        {
            Entity l_Entity(handle, &scene);
            SerializeEntity(l_Out, l_Entity);
        });

        l_Out << YAML::EndSeq;
        l_Out << YAML::EndMap;

        return std::string(l_Out.c_str());
    }

    static bool ParseYaml(Scene& scene, const YAML::Node& root)
    {
        if (!root["Scene"])
        {
            return false;
        }

        scene.SetName(root["Scene"].as<std::string>());

        const auto a_Entities = root["Entities"];
        if (!a_Entities)
        {
            return true;
        }

        for (const auto& it_EntityNode : a_Entities)
        {
            const uint64_t l_UUID = it_EntityNode["UUID"].as<uint64_t>();
            const std::string l_Tag = it_EntityNode["Tag"] ? it_EntityNode["Tag"].as<std::string>() : "Entity";
            Entity l_Entity = scene.CreateEntityWithUUID(l_UUID, l_Tag);

            const auto a_TransformNode = it_EntityNode["Transform"];
            if (a_TransformNode)
            {
                auto& a_TransformComponent = l_Entity.GetComponent<TransformComponent>();
                a_TransformComponent.Position = a_TransformNode["Position"].as<glm::vec3>();
                a_TransformComponent.Rotation = a_TransformNode["Rotation"].as<glm::vec3>();
                a_TransformComponent.Scale = a_TransformNode["Scale"].as<glm::vec3>();
                a_TransformComponent.ParentUUID = a_TransformNode["Parent"].as<uint64_t>(0);
            }

            const auto a_MeshNode = it_EntityNode["MeshComponent"];
            if (a_MeshNode)
            {
                const AssetHandle l_UUID = a_MeshNode["MeshAssetUUID"].as<uint64_t>(InvalidAsset);
                const std::string l_SourcePath = a_MeshNode["MeshSourcePath"].as<std::string>("");
                auto& a_MeshComponent = l_Entity.AddComponent<MeshComponent>();

                a_MeshComponent.MeshAssetUUID = l_UUID;
                if (l_UUID != InvalidAsset)
                {
                    AssetHandle l_FinalHandle = l_UUID;

                    if (!l_SourcePath.empty())
                    {
                        AssetHandle l_ImportedHandle = AssetRegistry::Get().ImportAsset(l_SourcePath);
                        if (l_ImportedHandle != InvalidAsset)
                        {
                            l_FinalHandle = l_ImportedHandle;
                        }
                    }

                    a_MeshComponent.MeshAssetUUID = l_FinalHandle;
                    a_MeshComponent.MeshData = AssetRegistry::Get().LoadMesh(l_FinalHandle);
                }
            }

            const auto a_CameraNode = it_EntityNode["Camera"];
            if (a_CameraNode)
            {
                auto& a_CameraComponent = l_Entity.AddComponent<CameraComponent>();
                a_CameraComponent.FOV = a_CameraNode["FOV"].as<float>(60.0f);
                a_CameraComponent.NearClip = a_CameraNode["NearClip"].as<float>(0.1f);
                a_CameraComponent.FarClip = a_CameraNode["FarClip"].as<float>(1000.0f);
                a_CameraComponent.Primary = a_CameraNode["Primary"].as<bool>(true);
            }

            const auto a_TextureNode = it_EntityNode["TextureComponent"];
            if (a_TextureNode)
            {
                const AssetHandle l_UUID = a_TextureNode["TextureAssetUUID"].as<uint64_t>(InvalidAsset);
                const std::string l_SourcePath = a_TextureNode["TextureSourcePath"].as<std::string>("");

                auto& a_TextureComponent = l_Entity.AddComponent<TextureComponent>();

                AssetHandle l_FinalHandle = l_UUID;

                if (!l_SourcePath.empty())
                {
                    AssetHandle l_ImportedHandle = AssetRegistry::Get().ImportAsset(l_SourcePath);
                    if (l_ImportedHandle != InvalidAsset)
                    {
                        l_FinalHandle = l_ImportedHandle;
                    }
                }

                a_TextureComponent.TextureAssetUUID = l_FinalHandle;
                a_TextureComponent.TextureData = AssetRegistry::Get().LoadTexture(l_FinalHandle);
            }
        }

        return true;
    }

    void SceneSerializer::Serialize(Scene& scene, const std::string& filepath)
    {
        std::ofstream l_File(filepath);
        if (!l_File.is_open())
        {
            return;
        }

        l_File << BuildYaml(scene);
    }

    std::string SceneSerializer::SerializeToString(Scene& scene)
    {
        return BuildYaml(scene);
    }

    bool SceneSerializer::Deserialize(Scene& scene, const std::string& filepath)
    {
        try
        {
            const YAML::Node l_Root = YAML::LoadFile(filepath);

            return ParseYaml(scene, l_Root);
        }
        catch (const YAML::Exception& e)
        {
            return false;
        }
    }

    bool SceneSerializer::DeserializeFromString(Scene& scene, const std::string& yaml)
    {
        try
        {
            const YAML::Node l_Root = YAML::Load(yaml);

            return ParseYaml(scene, l_Root);
        }
        catch (const YAML::Exception& e)
        {
            return false;
        }
    }
}