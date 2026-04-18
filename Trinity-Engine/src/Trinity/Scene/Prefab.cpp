#include "Trinity/Scene/Prefab.h"

#include "Trinity/Scene/Scene.h"
#include "Trinity/Scene/Entity.h"
#include "Trinity/Scene/Components/UUIDComponent.h"
#include "Trinity/Scene/Components/TagComponent.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Scene/Components/MeshComponent.h"
#include "Trinity/Scene/Components/CameraComponent.h"
#include "Trinity/Scene/Components/DirectionalLightComponent.h"
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

        static bool decode(const Node& l_Node, glm::vec3& v)
        {
            if (!l_Node.IsSequence() || l_Node.size() != 3)
            {
                return false;
            }

            v.x = l_Node[0].as<float>();
            v.y = l_Node[1].as<float>();
            v.z = l_Node[2].as<float>();

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

        const auto& l_Transform = entity.GetComponent<TransformComponent>();
        l_Out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
        l_Out << YAML::Key << "Position" << YAML::Value << l_Transform.Position;
        l_Out << YAML::Key << "Rotation" << YAML::Value << l_Transform.Rotation;
        l_Out << YAML::Key << "Scale"    << YAML::Value << l_Transform.Scale;
        l_Out << YAML::EndMap;

        if (entity.HasComponent<MeshComponent>())
        {
            const auto& l_Mesh = entity.GetComponent<MeshComponent>();
            l_Out << YAML::Key << "MeshComponent" << YAML::Value << YAML::BeginMap;
            l_Out << YAML::Key << "MeshAssetUUID" << YAML::Value << l_Mesh.MeshAssetUUID;
            l_Out << YAML::EndMap;
        }

        if (entity.HasComponent<CameraComponent>())
        {
            const auto& l_Camera = entity.GetComponent<CameraComponent>();
            l_Out << YAML::Key << "Camera" << YAML::Value << YAML::BeginMap;
            l_Out << YAML::Key << "FOV"      << YAML::Value << l_Camera.FOV;
            l_Out << YAML::Key << "NearClip" << YAML::Value << l_Camera.NearClip;
            l_Out << YAML::Key << "FarClip"  << YAML::Value << l_Camera.FarClip;
            l_Out << YAML::Key << "Primary"  << YAML::Value << l_Camera.Primary;
            l_Out << YAML::EndMap;
        }

        if (entity.HasComponent<DirectionalLightComponent>())
        {
            const auto& l_Light = entity.GetComponent<DirectionalLightComponent>();
            l_Out << YAML::Key << "DirectionalLight" << YAML::Value << YAML::BeginMap;
            l_Out << YAML::Key << "Color"     << YAML::Value << l_Light.Color;
            l_Out << YAML::Key << "Intensity" << YAML::Value << l_Light.Intensity;
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

        const auto l_TransformNode = l_Root["Transform"];
        if (l_TransformNode)
        {
            auto& l_Transform = l_Entity.GetComponent<TransformComponent>();
            l_Transform.Position = l_TransformNode["Position"].as<glm::vec3>();
            l_Transform.Rotation = l_TransformNode["Rotation"].as<glm::vec3>();
            l_Transform.Scale = l_TransformNode["Scale"].as<glm::vec3>();
        }

        const auto l_MeshNode = l_Root["MeshComponent"];
        if (l_MeshNode)
        {
            const AssetHandle l_UUID = l_MeshNode["MeshAssetUUID"].as<uint64_t>(InvalidAsset);
            auto& l_Mesh = l_Entity.AddComponent<MeshComponent>();
            l_Mesh.MeshAssetUUID = l_UUID;
            if (l_UUID != InvalidAsset)
            {
                l_Mesh.MeshData = AssetRegistry::Get().LoadMesh(l_UUID);
            }
        }

        const auto l_CameraNode = l_Root["Camera"];
        if (l_CameraNode)
        {
            auto& l_Camera = l_Entity.AddComponent<CameraComponent>();
            l_Camera.FOV = l_CameraNode["FOV"].as<float>(60.0f);
            l_Camera.NearClip = l_CameraNode["NearClip"].as<float>(0.1f);
            l_Camera.FarClip = l_CameraNode["FarClip"].as<float>(1000.0f);
            l_Camera.Primary = l_CameraNode["Primary"].as<bool>(true);
        }

        const auto l_LightNode = l_Root["DirectionalLight"];
        if (l_LightNode)
        {
            auto& l_Light = l_Entity.AddComponent<DirectionalLightComponent>();
            l_Light.Color = l_LightNode["Color"].as<glm::vec3>(glm::vec3(1.0f));
            l_Light.Intensity = l_LightNode["Intensity"].as<float>(1.0f);
        }

        return l_Entity;
    }
}
