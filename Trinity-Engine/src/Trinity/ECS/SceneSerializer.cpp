#include "Trinity/ECS/SceneSerializer.h"

#include "Trinity/ECS/Scene.h"
#include "Trinity/ECS/Entity.h"
#include "Trinity/ECS/Components.h"
#include "Trinity/Utilities/Log.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <filesystem>

namespace YAML
{
    template<>
    struct convert<glm::vec2>
    {
        static Node encode(const glm::vec2& v)
        {
            Node l_Node;
            l_Node.push_back(v.x);
            l_Node.push_back(v.y);
            l_Node.SetStyle(EmitterStyle::Flow);

            return l_Node;
        }

        static bool decode(const Node& l_Node, glm::vec2& v)
        {
            if (!l_Node.IsSequence() || l_Node.size() != 2)
            {
                return false;
            }
            
            v.x = l_Node[0].as<float>();
            v.y = l_Node[1].as<float>();

            return true;
        }
    };

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

    template<>
    struct convert<glm::vec4>
    {
        static Node encode(const glm::vec4& v)
        {
            Node l_Node;
            l_Node.push_back(v.x);
            l_Node.push_back(v.y);
            l_Node.push_back(v.z);
            l_Node.push_back(v.w);
            l_Node.SetStyle(EmitterStyle::Flow);

            return l_Node;
        }

        static bool decode(const Node& l_Node, glm::vec4& v)
        {
            if (!l_Node.IsSequence() || l_Node.size() != 4)
            {
                return false;
            }

            v.x = l_Node[0].as<float>();
            v.y = l_Node[1].as<float>();
            v.z = l_Node[2].as<float>();
            v.w = l_Node[3].as<float>();
            
            return true;
        }
    };
}

namespace Trinity
{
    static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
    {
        out << YAML::Flow << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;

        return out;
    }

    static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
    {
        out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        
        return out;
    }

    static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
    {
        out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
        
        return out;
    }

    static void SerializeEntity(YAML::Emitter& out, Entity entity)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "Entity" << YAML::Value << entity.GetComponent<IDComponent>().ID;

        if (entity.HasComponent<TagComponent>())
        {
            const TagComponent& l_Tag = entity.GetComponent<TagComponent>();
            out << YAML::Key << "TagComponent";
            out << YAML::BeginMap;
            out << YAML::Key << "Tag" << YAML::Value << l_Tag.Tag;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<TransformComponent>())
        {
            const TransformComponent& l_Transform = entity.GetComponent<TransformComponent>();
            out << YAML::Key << "TransformComponent";
            out << YAML::BeginMap;
            out << YAML::Key << "Translation" << YAML::Value << l_Transform.Translation;
            out << YAML::Key << "Rotation" << YAML::Value << l_Transform.Rotation;
            out << YAML::Key << "Scale" << YAML::Value << l_Transform.Scale;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<MeshRendererComponent>())
        {
            const MeshRendererComponent& l_Mesh = entity.GetComponent<MeshRendererComponent>();
            out << YAML::Key << "MeshRendererComponent";
            out << YAML::BeginMap;
            out << YAML::Key << "Primitive" << YAML::Value << static_cast<int>(l_Mesh.Primitive);
            out << YAML::Key << "Color" << YAML::Value << l_Mesh.Color;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<MaterialComponent>())
        {
            const MaterialComponent& l_Material = entity.GetComponent<MaterialComponent>();
            out << YAML::Key << "MaterialComponent";
            out << YAML::BeginMap;
            out << YAML::Key << "Name" << YAML::Value << (l_Material.Mat ? l_Material.Mat->GetName() : "");
            out << YAML::EndMap;
        }

        if (entity.HasComponent<CameraComponent>())
        {
            const CameraComponent& l_Camera = entity.GetComponent<CameraComponent>();
            out << YAML::Key << "CameraComponent";
            out << YAML::BeginMap;
            out << YAML::Key << "Projection" << YAML::Value << static_cast<int>(l_Camera.Projection);
            out << YAML::Key << "FieldOfViewDegrees" << YAML::Value << l_Camera.FieldOfViewDegrees;
            out << YAML::Key << "NearClip" << YAML::Value << l_Camera.NearClip;
            out << YAML::Key << "FarClip" << YAML::Value << l_Camera.FarClip;
            out << YAML::Key << "OrthoSize" << YAML::Value << l_Camera.OrthoSize;
            out << YAML::Key << "OrthoNear" << YAML::Value << l_Camera.OrthoNear;
            out << YAML::Key << "OrthoFar" << YAML::Value << l_Camera.OrthoFar;
            out << YAML::Key << "Primary" << YAML::Value << l_Camera.Primary;
            out << YAML::Key << "FixedAspectRatio" << YAML::Value << l_Camera.FixedAspectRatio;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<LightComponent>())
        {
            const LightComponent& l_Light = entity.GetComponent<LightComponent>();
            out << YAML::Key << "LightComponent";
            out << YAML::BeginMap;
            out << YAML::Key << "Type" << YAML::Value << static_cast<int>(l_Light.Type);
            out << YAML::Key << "Color" << YAML::Value << l_Light.Color;
            out << YAML::Key << "Intensity" << YAML::Value << l_Light.Intensity;
            out << YAML::Key << "Range" << YAML::Value << l_Light.Range;
            out << YAML::Key << "InnerConeAngleDegrees" << YAML::Value << l_Light.InnerConeAngleDegrees;
            out << YAML::Key << "OuterConeAngleDegrees" << YAML::Value << l_Light.OuterConeAngleDegrees;
            out << YAML::Key << "CastShadows" << YAML::Value << l_Light.CastShadows;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<RigidBodyComponent>())
        {
            const RigidBodyComponent& l_RigidBody = entity.GetComponent<RigidBodyComponent>();
            out << YAML::Key << "RigidBodyComponent";
            out << YAML::BeginMap;
            out << YAML::Key << "Type" << YAML::Value << static_cast<int>(l_RigidBody.Type);
            out << YAML::Key << "Mass" << YAML::Value << l_RigidBody.Mass;
            out << YAML::Key << "LinearDrag" << YAML::Value << l_RigidBody.LinearDrag;
            out << YAML::Key << "AngularDrag" << YAML::Value << l_RigidBody.AngularDrag;
            out << YAML::Key << "UseGravity" << YAML::Value << l_RigidBody.UseGravity;
            out << YAML::Key << "IsKinematic" << YAML::Value << l_RigidBody.IsKinematic;
            out << YAML::Key << "LinearVelocity" << YAML::Value << l_RigidBody.LinearVelocity;
            out << YAML::Key << "AngularVelocity" << YAML::Value << l_RigidBody.AngularVelocity;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<ScriptComponent>())
        {
            const ScriptComponent& l_Script = entity.GetComponent<ScriptComponent>();
            out << YAML::Key << "ScriptComponent";
            out << YAML::BeginMap;
            out << YAML::Key << "ScriptPath" << YAML::Value << l_Script.ScriptPath;
            out << YAML::Key << "Active" << YAML::Value << l_Script.Active;
            out << YAML::EndMap;
        }

        out << YAML::EndMap;
    }

    SceneSerializer::SceneSerializer(Scene& scene) : m_Scene(scene)
    {

    }

    void SceneSerializer::Serialize(const std::string& filePath)
    {
        YAML::Emitter l_Out;
        l_Out << YAML::BeginMap;
        l_Out << YAML::Key << "Scene" << YAML::Value << "Untitled";
        l_Out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        auto& a_Registry = m_Scene.GetRegistry();
        a_Registry.view<IDComponent>().each([&](entt::entity it_Handle, const IDComponent&)
            {
                Entity l_Entity(it_Handle, &a_Registry);
                if (!l_Entity)
                {
                    return;
                }
                SerializeEntity(l_Out, l_Entity);
            });

        l_Out << YAML::EndSeq;
        l_Out << YAML::EndMap;

        const std::filesystem::path l_Path(filePath);
        if (!l_Path.parent_path().empty())
        {
            std::error_code l_Error;
            std::filesystem::create_directories(l_Path.parent_path(), l_Error);
        }

        std::ofstream l_File(filePath);
        if (!l_File.is_open())
        {
            TR_CORE_ERROR("SceneSerializer::Serialize: failed to open file: {}", filePath);

            return;
        }

        l_File << l_Out.c_str();
    }

    bool SceneSerializer::Deserialize(const std::string& filePath)
    {
        YAML::Node l_Root;
        try
        {
            l_Root = YAML::LoadFile(filePath);
        }
        catch (const YAML::Exception& e)
        {
            TR_CORE_ERROR("SceneSerializer::Deserialize: YAML parse error in '{}': {}", filePath, e.what());

            return false;
        }

        if (!l_Root["Scene"])
        {
            TR_CORE_ERROR("SceneSerializer::Deserialize: missing 'Scene' key in '{}'", filePath);
            
            return false;
        }

        const YAML::Node l_Entities = l_Root["Entities"];
        if (!l_Entities)
        {
            return true;
        }

        for (const YAML::Node& it_EntityNode : l_Entities)
        {
            const uint64_t l_ID = it_EntityNode["Entity"].as<uint64_t>();

            std::string l_Name = "Entity";
            if (const YAML::Node& l_TagNode = it_EntityNode["TagComponent"])
            {
                l_Name = l_TagNode["Tag"].as<std::string>();
            }

            Entity l_Entity = m_Scene.CreateEntity(l_Name);
            l_Entity.GetComponent<IDComponent>().ID = l_ID;

            if (const YAML::Node& l_TransformNode = it_EntityNode["TransformComponent"])
            {
                TransformComponent& l_Transform = l_Entity.GetComponent<TransformComponent>();
                l_Transform.Translation = l_TransformNode["Translation"].as<glm::vec3>();
                l_Transform.Rotation = l_TransformNode["Rotation"].as<glm::vec3>();
                l_Transform.Scale = l_TransformNode["Scale"].as<glm::vec3>();
            }

            if (const YAML::Node& l_MeshNode = it_EntityNode["MeshRendererComponent"])
            {
                MeshRendererComponent& l_Mesh = l_Entity.AddComponent<MeshRendererComponent>();
                l_Mesh.Primitive = static_cast<Geometry::PrimitiveType>(l_MeshNode["Primitive"].as<int>());
                l_Mesh.Color = l_MeshNode["Color"].as<glm::vec4>();
            }

            if (const YAML::Node& l_MaterialNode = it_EntityNode["MaterialComponent"])
            {
                MaterialComponent& l_Material = l_Entity.AddComponent<MaterialComponent>();
                const std::string l_MatName = l_MaterialNode["Name"].as<std::string>();
                if (!l_MatName.empty())
                {
                    l_Material.Mat = std::make_shared<Material>(l_MatName);
                }
            }

            if (const YAML::Node& l_CameraNode = it_EntityNode["CameraComponent"])
            {
                CameraComponent& l_Camera = l_Entity.AddComponent<CameraComponent>();
                l_Camera.Projection = static_cast<ProjectionType>(l_CameraNode["Projection"].as<int>());
                l_Camera.FieldOfViewDegrees = l_CameraNode["FieldOfViewDegrees"].as<float>();
                l_Camera.NearClip = l_CameraNode["NearClip"].as<float>();
                l_Camera.FarClip = l_CameraNode["FarClip"].as<float>();
                l_Camera.OrthoSize = l_CameraNode["OrthoSize"].as<float>();
                l_Camera.OrthoNear = l_CameraNode["OrthoNear"].as<float>();
                l_Camera.OrthoFar = l_CameraNode["OrthoFar"].as<float>();
                l_Camera.Primary = l_CameraNode["Primary"].as<bool>();
                l_Camera.FixedAspectRatio = l_CameraNode["FixedAspectRatio"].as<bool>();
            }

            if (const YAML::Node& l_LightNode = it_EntityNode["LightComponent"])
            {
                LightComponent& l_Light = l_Entity.AddComponent<LightComponent>();
                l_Light.Type = static_cast<LightType>(l_LightNode["Type"].as<int>());
                l_Light.Color = l_LightNode["Color"].as<glm::vec3>();
                l_Light.Intensity = l_LightNode["Intensity"].as<float>();
                l_Light.Range = l_LightNode["Range"].as<float>();
                l_Light.InnerConeAngleDegrees = l_LightNode["InnerConeAngleDegrees"].as<float>();
                l_Light.OuterConeAngleDegrees = l_LightNode["OuterConeAngleDegrees"].as<float>();
                l_Light.CastShadows = l_LightNode["CastShadows"].as<bool>();
            }

            if (const YAML::Node& l_RigidBodyNode = it_EntityNode["RigidBodyComponent"])
            {
                RigidBodyComponent& l_RigidBody = l_Entity.AddComponent<RigidBodyComponent>();
                l_RigidBody.Type = static_cast<RigidBodyType>(l_RigidBodyNode["Type"].as<int>());
                l_RigidBody.Mass = l_RigidBodyNode["Mass"].as<float>();
                l_RigidBody.LinearDrag = l_RigidBodyNode["LinearDrag"].as<float>();
                l_RigidBody.AngularDrag = l_RigidBodyNode["AngularDrag"].as<float>();
                l_RigidBody.UseGravity = l_RigidBodyNode["UseGravity"].as<bool>();
                l_RigidBody.IsKinematic = l_RigidBodyNode["IsKinematic"].as<bool>();
                l_RigidBody.LinearVelocity = l_RigidBodyNode["LinearVelocity"].as<glm::vec3>();
                l_RigidBody.AngularVelocity = l_RigidBodyNode["AngularVelocity"].as<glm::vec3>();
            }

            if (const YAML::Node& l_ScriptNode = it_EntityNode["ScriptComponent"])
            {
                ScriptComponent& l_Script = l_Entity.AddComponent<ScriptComponent>();
                l_Script.ScriptPath = l_ScriptNode["ScriptPath"].as<std::string>();
                l_Script.Active = l_ScriptNode["Active"].as<bool>();
            }
        }

        return true;
    }
}