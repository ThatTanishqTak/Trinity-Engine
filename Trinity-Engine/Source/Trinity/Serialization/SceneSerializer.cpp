#include <Trinity/Serialization/SceneSerializer.h>

#include <fstream>
#include <sstream>
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
#include <Trinity/Scene/Components/LightComponent.h>
#include <Trinity/Scene/Components/AudioSourceComponent.h>
#include <Trinity/Scene/Components/AudioListenerComponent.h>
#include <Trinity/Scene/Components/Rigidbody2DComponent.h>
#include <Trinity/Scene/Components/BoxCollider2DComponent.h>
#include <Trinity/Scene/Components/CircleCollider2DComponent.h>
#include <Trinity/Assets/AssetDatabase.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    static constexpr uint32_t k_SceneVersion = 1;

    static const char* BodyTypeToString(BodyType type)
    {
        switch (type)
        {
        case BodyType::Static: return "Static";
        case BodyType::Kinematic: return "Kinematic";
        case BodyType::Dynamic: return "Dynamic";
        }

        return "Static";
    }

    static BodyType BodyTypeFromString(const std::string& value)
    {
        if (value == "Kinematic")
        {
            return BodyType::Kinematic;
        }

        if (value == "Dynamic")
        {
            return BodyType::Dynamic;
        }

        return BodyType::Static;
    }

    static void EmitPhysicsMaterial(YAML::Emitter& out, const PhysicsMaterial& material)
    {
        out << YAML::Key << "Friction" << YAML::Value << material.Friction;
        out << YAML::Key << "Restitution" << YAML::Value << material.Restitution;
        out << YAML::Key << "Density" << YAML::Value << material.Density;
        out << YAML::Key << "FrictionCombine" << YAML::Value << static_cast<uint32_t>(material.FrictionCombine);
        out << YAML::Key << "RestitutionCombine" << YAML::Value << static_cast<uint32_t>(material.RestitutionCombine);
    }

    static void ReadPhysicsMaterial(const YAML::Node& node, PhysicsMaterial& material)
    {
        if (node["Friction"])
        {
            material.Friction = node["Friction"].as<float>();
        }

        if (node["Restitution"])
        {
            material.Restitution = node["Restitution"].as<float>();
        }

        if (node["Density"])
        {
            material.Density = node["Density"].as<float>();
        }

        if (node["FrictionCombine"])
        {
            material.FrictionCombine = static_cast<PhysicsCombineMode>(node["FrictionCombine"].as<uint32_t>());
        }

        if (node["RestitutionCombine"])
        {
            material.RestitutionCombine = static_cast<PhysicsCombineMode>(node["RestitutionCombine"].as<uint32_t>());
        }
    }

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

            if (!l_MeshRenderer->Materials.empty())
            {
                out << YAML::Key << "Materials" << YAML::Value << YAML::Flow << YAML::BeginSeq;
                for (const UUID& it_Material : l_MeshRenderer->Materials)
                {
                    out << static_cast<uint64_t>(it_Material);
                }
                out << YAML::EndSeq;
            }

            out << YAML::EndMap;
        }

        if (const CameraComponent* l_Camera = l_Registry.try_get<CameraComponent>(handle))
        {
            out << YAML::Key << "Camera" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Primary" << YAML::Value << l_Camera->Primary;
            out << YAML::EndMap;
        }

        if (const LightComponent* l_Light = l_Registry.try_get<LightComponent>(handle))
        {
            out << YAML::Key << "Light" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Type" << YAML::Value << LightTypeToString(l_Light->Type);
            out << YAML::Key << "Color" << YAML::Value << l_Light->Color;
            out << YAML::Key << "Intensity" << YAML::Value << l_Light->Intensity;
            out << YAML::Key << "Range" << YAML::Value << l_Light->Range;
            out << YAML::Key << "InnerConeAngle" << YAML::Value << l_Light->InnerConeAngle;
            out << YAML::Key << "OuterConeAngle" << YAML::Value << l_Light->OuterConeAngle;
            out << YAML::EndMap;
        }

        if (const AudioSourceComponent* l_AudioSource = l_Registry.try_get<AudioSourceComponent>(handle))
        {
            out << YAML::Key << "AudioSource" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Clip" << YAML::Value << static_cast<uint64_t>(l_AudioSource->Clip);
            out << YAML::Key << "Volume" << YAML::Value << l_AudioSource->Volume;
            out << YAML::Key << "Pitch" << YAML::Value << l_AudioSource->Pitch;
            out << YAML::Key << "Loop" << YAML::Value << l_AudioSource->Loop;
            out << YAML::Key << "PlayOnStart" << YAML::Value << l_AudioSource->PlayOnStart;
            out << YAML::Key << "Spatial" << YAML::Value << l_AudioSource->Spatial;
            out << YAML::EndMap;
        }

        if (const AudioListenerComponent* l_AudioListener = l_Registry.try_get<AudioListenerComponent>(handle))
        {
            out << YAML::Key << "AudioListener" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Active" << YAML::Value << l_AudioListener->Active;
            out << YAML::EndMap;
        }

        if (const Rigidbody2DComponent* l_Rigidbody = l_Registry.try_get<Rigidbody2DComponent>(handle))
        {
            out << YAML::Key << "Rigidbody2D" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Type" << YAML::Value << BodyTypeToString(l_Rigidbody->Type);
            out << YAML::Key << "FixedRotation" << YAML::Value << l_Rigidbody->FixedRotation;
            out << YAML::Key << "GravityScale" << YAML::Value << l_Rigidbody->GravityScale;
            out << YAML::Key << "LinearDamping" << YAML::Value << l_Rigidbody->LinearDamping;
            out << YAML::Key << "AngularDamping" << YAML::Value << l_Rigidbody->AngularDamping;
            out << YAML::Key << "Layer" << YAML::Value << l_Rigidbody->Layer;
            out << YAML::EndMap;
        }

        if (const BoxCollider2DComponent* l_BoxCollider = l_Registry.try_get<BoxCollider2DComponent>(handle))
        {
            out << YAML::Key << "BoxCollider2D" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Offset" << YAML::Value << l_BoxCollider->Offset;
            out << YAML::Key << "HalfExtents" << YAML::Value << l_BoxCollider->HalfExtents;
            out << YAML::Key << "IsTrigger" << YAML::Value << l_BoxCollider->IsTrigger;
            EmitPhysicsMaterial(out, l_BoxCollider->Material);
            out << YAML::EndMap;
        }

        if (const CircleCollider2DComponent* l_CircleCollider = l_Registry.try_get<CircleCollider2DComponent>(handle))
        {
            out << YAML::Key << "CircleCollider2D" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Offset" << YAML::Value << l_CircleCollider->Offset;
            out << YAML::Key << "Radius" << YAML::Value << l_CircleCollider->Radius;
            out << YAML::Key << "IsTrigger" << YAML::Value << l_CircleCollider->IsTrigger;
            EmitPhysicsMaterial(out, l_CircleCollider->Material);
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

            if (YAML::Node l_RotationNode = l_TransformNode["Rotation"])
            {
                if (l_RotationNode.IsSequence() && l_RotationNode.size() == 4)
                {
                    l_Transform.Rotation = l_RotationNode.as<glm::quat>();
                }
                else
                {
                    // Version 1 files stored Euler radians; glm::quat(euler) is the exact rotation the old GetLocalMatrix produced
                    l_Transform.SetEulerAngles(l_RotationNode.as<glm::vec3>());
                }
            }

            l_Transform.Scale = l_TransformNode["Scale"].as<glm::vec3>();
        }

        if (YAML::Node l_MeshNode = node["MeshRenderer"])
        {
            MeshRendererComponent l_Component;
            if (l_MeshNode["MeshAsset"])
            {
                l_Component.MeshAsset = UUID(l_MeshNode["MeshAsset"].as<uint64_t>());
            }

            if (YAML::Node l_Materials = l_MeshNode["Materials"])
            {
                for (const YAML::Node& it_Material : l_Materials)
                {
                    l_Component.Materials.emplace_back(it_Material.as<uint64_t>());
                }
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

        if (YAML::Node l_LightNode = node["Light"])
        {
            LightComponent l_Component;
            if (l_LightNode["Type"])
            {
                l_Component.Type = LightTypeFromString(l_LightNode["Type"].as<std::string>());
            }

            if (l_LightNode["Color"])
            {
                l_Component.Color = l_LightNode["Color"].as<glm::vec3>();
            }

            if (l_LightNode["Intensity"])
            {
                l_Component.Intensity = l_LightNode["Intensity"].as<float>();
            }

            if (l_LightNode["Range"])
            {
                l_Component.Range = l_LightNode["Range"].as<float>();
            }

            if (l_LightNode["InnerConeAngle"])
            {
                l_Component.InnerConeAngle = l_LightNode["InnerConeAngle"].as<float>();
            }

            if (l_LightNode["OuterConeAngle"])
            {
                l_Component.OuterConeAngle = l_LightNode["OuterConeAngle"].as<float>();
            }

            entity.AddComponent<LightComponent>(l_Component);
        }

        if (YAML::Node l_AudioSourceNode = node["AudioSource"])
        {
            AudioSourceComponent l_Component;
            if (l_AudioSourceNode["Clip"])
            {
                l_Component.Clip = UUID(l_AudioSourceNode["Clip"].as<uint64_t>());
            }

            if (l_AudioSourceNode["Volume"])
            {
                l_Component.Volume = l_AudioSourceNode["Volume"].as<float>();
            }

            if (l_AudioSourceNode["Pitch"])
            {
                l_Component.Pitch = l_AudioSourceNode["Pitch"].as<float>();
            }

            if (l_AudioSourceNode["Loop"])
            {
                l_Component.Loop = l_AudioSourceNode["Loop"].as<bool>();
            }

            if (l_AudioSourceNode["PlayOnStart"])
            {
                l_Component.PlayOnStart = l_AudioSourceNode["PlayOnStart"].as<bool>();
            }

            if (l_AudioSourceNode["Spatial"])
            {
                l_Component.Spatial = l_AudioSourceNode["Spatial"].as<bool>();
            }

            entity.AddComponent<AudioSourceComponent>(l_Component);
        }

        if (YAML::Node l_AudioListenerNode = node["AudioListener"])
        {
            AudioListenerComponent l_Component;
            if (l_AudioListenerNode["Active"])
            {
                l_Component.Active = l_AudioListenerNode["Active"].as<bool>();
            }

            entity.AddComponent<AudioListenerComponent>(l_Component);
        }

        if (YAML::Node l_RigidbodyNode = node["Rigidbody2D"])
        {
            Rigidbody2DComponent l_Component;
            if (l_RigidbodyNode["Type"])
            {
                l_Component.Type = BodyTypeFromString(l_RigidbodyNode["Type"].as<std::string>());
            }

            if (l_RigidbodyNode["FixedRotation"])
            {
                l_Component.FixedRotation = l_RigidbodyNode["FixedRotation"].as<bool>();
            }

            if (l_RigidbodyNode["GravityScale"])
            {
                l_Component.GravityScale = l_RigidbodyNode["GravityScale"].as<float>();
            }

            if (l_RigidbodyNode["LinearDamping"])
            {
                l_Component.LinearDamping = l_RigidbodyNode["LinearDamping"].as<float>();
            }

            if (l_RigidbodyNode["AngularDamping"])
            {
                l_Component.AngularDamping = l_RigidbodyNode["AngularDamping"].as<float>();
            }

            if (l_RigidbodyNode["Layer"])
            {
                l_Component.Layer = l_RigidbodyNode["Layer"].as<uint32_t>();
            }

            entity.AddComponent<Rigidbody2DComponent>(l_Component);
        }

        if (YAML::Node l_BoxColliderNode = node["BoxCollider2D"])
        {
            BoxCollider2DComponent l_Component;
            if (l_BoxColliderNode["Offset"])
            {
                l_Component.Offset = l_BoxColliderNode["Offset"].as<glm::vec2>();
            }

            if (l_BoxColliderNode["HalfExtents"])
            {
                l_Component.HalfExtents = l_BoxColliderNode["HalfExtents"].as<glm::vec2>();
            }

            if (l_BoxColliderNode["IsTrigger"])
            {
                l_Component.IsTrigger = l_BoxColliderNode["IsTrigger"].as<bool>();
            }

            ReadPhysicsMaterial(l_BoxColliderNode, l_Component.Material);
            entity.AddComponent<BoxCollider2DComponent>(l_Component);
        }

        if (YAML::Node l_CircleColliderNode = node["CircleCollider2D"])
        {
            CircleCollider2DComponent l_Component;
            if (l_CircleColliderNode["Offset"])
            {
                l_Component.Offset = l_CircleColliderNode["Offset"].as<glm::vec2>();
            }

            if (l_CircleColliderNode["Radius"])
            {
                l_Component.Radius = l_CircleColliderNode["Radius"].as<float>();
            }

            if (l_CircleColliderNode["IsTrigger"])
            {
                l_Component.IsTrigger = l_CircleColliderNode["IsTrigger"].as<bool>();
            }

            ReadPhysicsMaterial(l_CircleColliderNode, l_Component.Material);
            entity.AddComponent<CircleCollider2DComponent>(l_Component);
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

    std::string SceneSerializer::SerializeToString(Scene& scene, const std::string& sceneName)
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

        return std::string(l_Out.c_str());
    }

    bool SceneSerializer::Serialize(Scene& scene, const std::filesystem::path& path, const std::string& sceneName)
    {
        std::string l_Data = SerializeToString(scene, sceneName);

        std::error_code l_DirectoryError;
        std::filesystem::create_directories(path.parent_path(), l_DirectoryError);

        std::ofstream l_Stream(path);
        if (!l_Stream.is_open())
        {
            return false;
        }

        l_Stream << l_Data;

        return true;
    }

    bool SceneSerializer::Deserialize(Scene& scene, AssetDatabase& assetDatabase, const std::filesystem::path& path)
    {
        std::ifstream l_Stream(path);
        if (!l_Stream.is_open())
        {
            TR_CORE_ERROR("Failed to open scene file: {}", path.string());

            return false;
        }

        std::stringstream l_Buffer;
        l_Buffer << l_Stream.rdbuf();

        return DeserializeFromString(scene, assetDatabase, l_Buffer.str());
    }

    bool SceneSerializer::DeserializeFromString(Scene& scene, AssetDatabase& assetDatabase, const std::string& data)
    {
        try
        {
            YAML::Node l_Root = YAML::Load(data);

            if (!l_Root["Version"])
            {
                return false;
            }

            uint32_t l_Version = l_Root["Version"].as<uint32_t>();
            if (l_Version != k_SceneVersion)
            {

            }

            YAML::Node l_Entities = l_Root["Entities"];
            if (!l_Entities)
            {


                return true;
            }

            std::unordered_map<uint64_t, Entity> l_EntityMap;
            std::vector<std::pair<uint64_t, uint64_t>> l_PendingParents;

            for (const YAML::Node& l_EntityNode : l_Entities)
            {
                if (!l_EntityNode["UUID"])
                {


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

                }
            }



            return true;
        }
        catch (const std::exception& exception)
        {
            TR_CORE_CRITICAL("{}", exception.what());

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
        catch (const std::exception& exception)
        {
            TR_CORE_CRITICAL("{}", exception.what());

            return Entity();
        }
    }
}