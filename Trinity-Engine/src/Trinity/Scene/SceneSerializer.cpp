#include "Trinity/Scene/SceneSerializer.h"

#include "Trinity/Scene/Scene.h"
#include "Trinity/Scene/Entity.h"
#include "Trinity/Scene/Components/UUIDComponent.h"
#include "Trinity/Scene/Components/TagComponent.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Scene/Components/CameraComponent.h"
#include "Trinity/Scene/Components/DirectionalLightComponent.h"
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
                return false;
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

    static void SerializeEntity(YAML::Emitter& out, Entity entity)
    {
        out << YAML::BeginMap;

        out << YAML::Key << "UUID" << YAML::Value << entity.GetComponent<UUIDComponent>().UUID;
        out << YAML::Key << "Tag"  << YAML::Value << entity.GetComponent<TagComponent>().Tag;

        const auto& l_Transform = entity.GetComponent<TransformComponent>();
        out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Position" << YAML::Value << l_Transform.Position;
        out << YAML::Key << "Rotation" << YAML::Value << l_Transform.Rotation;
        out << YAML::Key << "Scale"    << YAML::Value << l_Transform.Scale;
        out << YAML::Key << "Parent"   << YAML::Value << l_Transform.ParentUUID;
        out << YAML::EndMap;

        if (entity.HasComponent<CameraComponent>())
        {
            const auto& l_Camera = entity.GetComponent<CameraComponent>();
            out << YAML::Key << "Camera" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "FOV"      << YAML::Value << l_Camera.FOV;
            out << YAML::Key << "NearClip" << YAML::Value << l_Camera.NearClip;
            out << YAML::Key << "FarClip"  << YAML::Value << l_Camera.FarClip;
            out << YAML::Key << "Primary"  << YAML::Value << l_Camera.Primary;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<DirectionalLightComponent>())
        {
            const auto& l_Light = entity.GetComponent<DirectionalLightComponent>();
            out << YAML::Key << "DirectionalLight" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Color"     << YAML::Value << l_Light.Color;
            out << YAML::Key << "Intensity" << YAML::Value << l_Light.Intensity;
            out << YAML::EndMap;
        }

        out << YAML::EndMap;
    }

    static std::string BuildYaml(Scene& scene)
    {
        YAML::Emitter l_Out;
        l_Out << YAML::BeginMap;
        l_Out << YAML::Key << "Scene"    << YAML::Value << scene.GetName();
        l_Out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        scene.GetRegistry().view<UUIDComponent>().each([&](entt::entity l_Handle, const UUIDComponent&)
        {
            Entity l_Entity(l_Handle, &scene);
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
            TR_CORE_ERROR("SceneSerializer: missing 'Scene' key");
            return false;
        }

        scene.SetName(root["Scene"].as<std::string>());

        const auto l_Entities = root["Entities"];
        if (!l_Entities)
            return true;

        for (const auto& l_EntityNode : l_Entities)
        {
            const uint64_t l_UUID = l_EntityNode["UUID"].as<uint64_t>();
            const std::string l_Tag = l_EntityNode["Tag"] ? l_EntityNode["Tag"].as<std::string>() : "Entity";

            Entity l_Entity = scene.CreateEntityWithUUID(l_UUID, l_Tag);

            const auto l_TransformNode = l_EntityNode["Transform"];
            if (l_TransformNode)
            {
                auto& l_Transform = l_Entity.GetComponent<TransformComponent>();
                l_Transform.Position   = l_TransformNode["Position"].as<glm::vec3>();
                l_Transform.Rotation   = l_TransformNode["Rotation"].as<glm::vec3>();
                l_Transform.Scale      = l_TransformNode["Scale"].as<glm::vec3>();
                l_Transform.ParentUUID = l_TransformNode["Parent"].as<uint64_t>(0);
            }

            const auto l_CameraNode = l_EntityNode["Camera"];
            if (l_CameraNode)
            {
                auto& l_Camera = l_Entity.AddComponent<CameraComponent>();
                l_Camera.FOV      = l_CameraNode["FOV"].as<float>(60.0f);
                l_Camera.NearClip = l_CameraNode["NearClip"].as<float>(0.1f);
                l_Camera.FarClip  = l_CameraNode["FarClip"].as<float>(1000.0f);
                l_Camera.Primary  = l_CameraNode["Primary"].as<bool>(true);
            }

            const auto l_LightNode = l_EntityNode["DirectionalLight"];
            if (l_LightNode)
            {
                auto& l_Light = l_Entity.AddComponent<DirectionalLightComponent>();
                l_Light.Color     = l_LightNode["Color"].as<glm::vec3>(glm::vec3(1.0f));
                l_Light.Intensity = l_LightNode["Intensity"].as<float>(1.0f);
            }
        }

        return true;
    }

    void SceneSerializer::Serialize(Scene& scene, const std::string& filepath)
    {
        std::ofstream l_File(filepath);
        if (!l_File.is_open())
        {
            TR_CORE_ERROR("SceneSerializer: cannot open '{}' for writing", filepath);
            return;
        }

        l_File << BuildYaml(scene);
        TR_CORE_INFO("Scene saved: {}", filepath);
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
            TR_CORE_ERROR("SceneSerializer: failed to load '{}': {}", filepath, e.what());
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
            TR_CORE_ERROR("SceneSerializer: failed to parse yaml: {}", e.what());
            return false;
        }
    }
}
