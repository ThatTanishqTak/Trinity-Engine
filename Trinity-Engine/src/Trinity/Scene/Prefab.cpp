#include "Trinity/Scene/Prefab.h"

#include "Trinity/Scene/Scene.h"
#include "Trinity/Scene/Entity.h"
#include "Trinity/Scene/Components/UUIDComponent.h"
#include "Trinity/Scene/Components/TagComponent.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Scene/Components/MeshComponent.h"
#include "Trinity/Scene/Components/CameraComponent.h"
#include "Trinity/Scene/Components/LightComponent.h"
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

    void Prefab::Save(Entity entity, const std::string& path)
    {
        YAML::Emitter l_Out;
        l_Out << YAML::BeginMap;
        l_Out << YAML::Key << "Prefab" << YAML::Value << entity.GetComponent<TagComponent>().Tag;

        l_Out << YAML::Key << "Tag" << YAML::Value << entity.GetComponent<TagComponent>().Tag;

        const auto& a_TransformComponent = entity.GetComponent<TransformComponent>();
        l_Out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
        l_Out << YAML::Key << "Position" << YAML::Value << a_TransformComponent.Position;
        l_Out << YAML::Key << "Rotation" << YAML::Value << a_TransformComponent.Rotation;
        l_Out << YAML::Key << "Scale" << YAML::Value << a_TransformComponent.Scale;
        l_Out << YAML::EndMap;

        if (entity.HasComponent<MeshComponent>())
        {
            const auto& a_MeshComponent = entity.GetComponent<MeshComponent>();
            l_Out << YAML::Key << "MeshComponent" << YAML::Value << YAML::BeginMap;
            l_Out << YAML::Key << "MeshAssetUUID" << YAML::Value << a_MeshComponent.MeshAssetUUID;
            l_Out << YAML::EndMap;
        }

        if (entity.HasComponent<CameraComponent>())
        {
            const auto& a_CameraComponent = entity.GetComponent<CameraComponent>();
            l_Out << YAML::Key << "Camera" << YAML::Value << YAML::BeginMap;
            l_Out << YAML::Key << "FOV" << YAML::Value << a_CameraComponent.FOV;
            l_Out << YAML::Key << "NearClip" << YAML::Value << a_CameraComponent.NearClip;
            l_Out << YAML::Key << "FarClip" << YAML::Value << a_CameraComponent.FarClip;
            l_Out << YAML::Key << "Primary" << YAML::Value << a_CameraComponent.Primary;
            l_Out << YAML::EndMap;
        }

        if (entity.HasComponent<DirectionalLightComponent>())
        {
            const auto& a_DirectionalLightComponent = entity.GetComponent<DirectionalLightComponent>();
            l_Out << YAML::Key << "DirectionalLight" << YAML::Value << YAML::BeginMap;
            l_Out << YAML::Key << "Color" << YAML::Value << a_DirectionalLightComponent.Color;
            l_Out << YAML::Key << "Intensity" << YAML::Value << a_DirectionalLightComponent.Intensity;
            l_Out << YAML::EndMap;
        }

        l_Out << YAML::EndMap;

        std::ofstream l_File(path);
        if (!l_File.is_open())
        {
            TR_CORE_ERROR("Prefab: cannot write '{}'", path);
            return;
        }

        l_File << l_Out.c_str();
        TR_CORE_INFO("Prefab saved: {}", path);
    }

    Entity Prefab::Instantiate(Scene& scene, const std::string& path)
    {
        YAML::Node l_Root;
        try
        {
            l_Root = YAML::LoadFile(path);
        }
        catch (const YAML::Exception& e)
        {
            TR_CORE_ERROR("Prefab: failed to load '{}': {}", path, e.what());
            return Entity{};
        }

        if (!l_Root["Prefab"])
        {
            TR_CORE_ERROR("Prefab: missing 'Prefab' key in '{}'", path);
            return Entity{};
        }

        const std::string l_Tag = l_Root["Tag"] ? l_Root["Tag"].as<std::string>() : "Prefab";
        Entity l_Entity = scene.CreateEntity(l_Tag);

        const auto a_TransformNode = l_Root["Transform"];
        if (a_TransformNode)
        {
            auto& a_TransformComponent = l_Entity.GetComponent<TransformComponent>();
            a_TransformComponent.Position = a_TransformNode["Position"].as<glm::vec3>();
            a_TransformComponent.Rotation = a_TransformNode["Rotation"].as<glm::vec3>();
            a_TransformComponent.Scale = a_TransformNode["Scale"].as<glm::vec3>();
        }

        const auto a_MeshNode = l_Root["MeshComponent"];
        if (a_MeshNode)
        {
            const AssetHandle l_UUID = a_MeshNode["MeshAssetUUID"].as<uint64_t>(InvalidAsset);
            auto& a_MeshComponent = l_Entity.AddComponent<MeshComponent>();

            a_MeshComponent.MeshAssetUUID = l_UUID;
            if (l_UUID != InvalidAsset)
            {
                a_MeshComponent.MeshData = AssetRegistry::Get().LoadMesh(l_UUID);
            }
        }

        const auto a_CameraNode = l_Root["Camera"];
        if (a_CameraNode)
        {
            auto& a_CameraComponent = l_Entity.AddComponent<CameraComponent>();
            a_CameraComponent.FOV = a_CameraNode["FOV"].as<float>(60.0f);
            a_CameraComponent.NearClip = a_CameraNode["NearClip"].as<float>(0.1f);
            a_CameraComponent.FarClip = a_CameraNode["FarClip"].as<float>(1000.0f);
            a_CameraComponent.Primary = a_CameraNode["Primary"].as<bool>(true);
        }

        const auto a_DirectionalLightNode = l_Root["DirectionalLight"];
        if (a_DirectionalLightNode)
        {
            auto& a_DirectionalLightComponent = l_Entity.AddComponent<DirectionalLightComponent>();
            a_DirectionalLightComponent.Color = a_DirectionalLightNode["Color"].as<glm::vec3>(glm::vec3(1.0f));
            a_DirectionalLightComponent.Intensity = a_DirectionalLightNode["Intensity"].as<float>(1.0f);
        }

        return l_Entity;
    }
}