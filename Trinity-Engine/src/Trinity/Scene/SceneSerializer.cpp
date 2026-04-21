#include "Trinity/Scene/SceneSerializer.h"

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

        if (entity.HasComponent<LightComponent>())
        {
            const auto& l_Light = entity.GetComponent<LightComponent>();

            out << YAML::Key << "Light" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Type" << YAML::Value << GetLightTypeName(GetLightType(l_Light));

            std::visit([&out](const auto& variant)
            {
                using T = std::decay_t<decltype(variant)>;

                out << YAML::Key << "Color" << YAML::Value << variant.Color;
                out << YAML::Key << "Intensity" << YAML::Value << variant.Intensity;

                if constexpr (std::is_same_v<T, PointLight>)
                {
                    out << YAML::Key << "Range" << YAML::Value << variant.Range;
                }
                else if constexpr (std::is_same_v<T, SpotLight>)
                {
                    out << YAML::Key << "Range" << YAML::Value << variant.Range;
                    out << YAML::Key << "InnerConeAngle" << YAML::Value << variant.InnerConeAngle;
                    out << YAML::Key << "OuterConeAngle" << YAML::Value << variant.OuterConeAngle;
                }
                else if constexpr (std::is_same_v<T, RectLight>)
                {
                    out << YAML::Key << "Width" << YAML::Value << variant.Width;
                    out << YAML::Key << "Height" << YAML::Value << variant.Height;
                }
                else if constexpr (std::is_same_v<T, CapsuleLight>)
                {
                    out << YAML::Key << "Length" << YAML::Value << variant.Length;
                    out << YAML::Key << "Radius" << YAML::Value << variant.Radius;
                }
                else if constexpr (std::is_same_v<T, SphereLight>)
                {
                    out << YAML::Key << "Radius" << YAML::Value << variant.Radius;
                }
            }, l_Light.Data);

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
            TR_CORE_ERROR("SceneSerializer: missing 'Scene' key");
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
                    if (!l_SourcePath.empty())
                    {
                        AssetRegistry::Get().ImportAsset(l_SourcePath);
                    }

                    a_MeshComponent.MeshData = AssetRegistry::Get().LoadMesh(l_UUID);
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

            const auto a_LightNode = it_EntityNode["Light"];
            if (a_LightNode)
            {
                auto& a_Light = l_Entity.AddComponent<LightComponent>();

                const std::string l_TypeName = a_LightNode["Type"].as<std::string>("Directional");
                const glm::vec3 l_Color = a_LightNode["Color"].as<glm::vec3>(glm::vec3(1.0f));
                const float l_Intensity = a_LightNode["Intensity"].as<float>(1.0f);

                if (l_TypeName == "Point")
                {
                    PointLight l_Data{ l_Color, l_Intensity, a_LightNode["Range"].as<float>(10.0f) };
                    a_Light.Data = l_Data;
                }
                else if (l_TypeName == "Spot")
                {
                    SpotLight l_Data{};
                    l_Data.Color = l_Color;
                    l_Data.Intensity = l_Intensity;
                    l_Data.Range = a_LightNode["Range"].as<float>(10.0f);
                    l_Data.InnerConeAngle = a_LightNode["InnerConeAngle"].as<float>(0.6108652f);
                    l_Data.OuterConeAngle = a_LightNode["OuterConeAngle"].as<float>(0.7853982f);
                    a_Light.Data = l_Data;
                }
                else if (l_TypeName == "Rect")
                {
                    RectLight l_Data{};
                    l_Data.Color = l_Color;
                    l_Data.Intensity = l_Intensity;
                    l_Data.Width = a_LightNode["Width"].as<float>(1.0f);
                    l_Data.Height = a_LightNode["Height"].as<float>(1.0f);
                    a_Light.Data = l_Data;
                }
                else if (l_TypeName == "Capsule")
                {
                    CapsuleLight l_Data{};
                    l_Data.Color = l_Color;
                    l_Data.Intensity = l_Intensity;
                    l_Data.Length = a_LightNode["Length"].as<float>(1.0f);
                    l_Data.Radius = a_LightNode["Radius"].as<float>(0.1f);
                    a_Light.Data = l_Data;
                }
                else if (l_TypeName == "Sphere")
                {
                    SphereLight l_Data{};
                    l_Data.Color = l_Color;
                    l_Data.Intensity = l_Intensity;
                    l_Data.Radius = a_LightNode["Radius"].as<float>(0.5f);
                    a_Light.Data = l_Data;
                }
                else
                {
                    a_Light.Data = DirectionalLight{ l_Color, l_Intensity };
                }
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