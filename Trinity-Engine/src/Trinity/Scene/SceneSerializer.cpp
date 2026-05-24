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
#include "Trinity/Scene/Components/MaterialComponent.h"
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

        static bool decode(const Node& node, glm::vec4& v)
        {
            if (!node.IsSequence() || node.size() != 4)
            {
                return false;
            }

            v.x = node[0].as<float>();
            v.y = node[1].as<float>();
            v.z = node[2].as<float>();
            v.w = node[3].as<float>();

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

    static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
    {
        out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;

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

        if (entity.HasComponent<MaterialComponent>())
        {
            const auto& a_MaterialComponent = entity.GetComponent<MaterialComponent>();
            const auto& a_Properties = a_MaterialComponent.GetEditableProperties();

            out << YAML::Key << "MaterialComponent" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "MaterialAssetUUID" << YAML::Value << a_MaterialComponent.MaterialAssetUUID;
            out << YAML::Key << "UseOverrideProperties" << YAML::Value << a_MaterialComponent.UseOverrideProperties;
            out << YAML::Key << "Name" << YAML::Value << (a_MaterialComponent.MaterialData ? a_MaterialComponent.MaterialData->GetName() : std::string{});

            out << YAML::Key << "OverrideProperties" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "BaseColor" << YAML::Value << a_Properties.BaseColor;
            out << YAML::Key << "Metallic" << YAML::Value << a_Properties.Metallic;
            out << YAML::Key << "Roughness" << YAML::Value << a_Properties.Roughness;
            out << YAML::Key << "AmbientOcclusion" << YAML::Value << a_Properties.AmbientOcclusion;
            out << YAML::Key << "EmissiveColor" << YAML::Value << a_Properties.EmissiveColor;
            out << YAML::Key << "EmissiveStrength" << YAML::Value << a_Properties.EmissiveStrength;
            out << YAML::Key << "AlphaCutoff" << YAML::Value << a_Properties.AlphaCutoff;
            out << YAML::Key << "AlphaMode" << YAML::Value << static_cast<uint32_t>(a_Properties.AlphaMode);

            auto EmitTextureSlot = [&out](const char* key, const MaterialTextureSlot& slot)
            {
                out << YAML::Key << key << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "TextureAssetUUID" << YAML::Value << slot.TextureAssetUUID;

                const AssetMetadata* l_MetaData = AssetRegistry::Get().GetMetadata(slot.TextureAssetUUID);
                out << YAML::Key << "TextureSourcePath" << YAML::Value << (l_MetaData ? l_MetaData->SourcePath : std::string{});
                out << YAML::Key << "Enabled" << YAML::Value << slot.Enabled;
                out << YAML::EndMap;
            };

            EmitTextureSlot("AlbedoTexture", a_Properties.AlbedoTexture);
            EmitTextureSlot("NormalTexture", a_Properties.NormalTexture);
            EmitTextureSlot("MetallicRoughnessTexture", a_Properties.MetallicRoughnessTexture);
            EmitTextureSlot("AmbientOcclusionTexture", a_Properties.AmbientOcclusionTexture);
            EmitTextureSlot("EmissiveTexture", a_Properties.EmissiveTexture);

            out << YAML::EndMap;

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

            const auto a_MaterialNode = it_EntityNode["MaterialComponent"];
            if (a_MaterialNode)
            {
                auto& a_MaterialComponent = l_Entity.AddComponent<MaterialComponent>();
                a_MaterialComponent.MaterialAssetUUID = a_MaterialNode["MaterialAssetUUID"].as<uint64_t>(InvalidAsset);
                a_MaterialComponent.UseOverrideProperties = a_MaterialNode["UseOverrideProperties"].as<bool>(true);

                const std::string l_Name = a_MaterialNode["Name"].as<std::string>("Default Material");
                a_MaterialComponent.MaterialData = std::make_shared<Material>(l_Name);

                MaterialProperties l_Properties{};

                const auto a_PropertiesNode = a_MaterialNode["OverrideProperties"];
                if (a_PropertiesNode)
                {
                    l_Properties.BaseColor = a_PropertiesNode["BaseColor"].as<glm::vec4>(glm::vec4(1.0f));
                    l_Properties.Metallic = a_PropertiesNode["Metallic"].as<float>(0.0f);
                    l_Properties.Roughness = a_PropertiesNode["Roughness"].as<float>(0.5f);
                    l_Properties.AmbientOcclusion = a_PropertiesNode["AmbientOcclusion"].as<float>(1.0f);
                    l_Properties.EmissiveColor = a_PropertiesNode["EmissiveColor"].as<glm::vec3>(glm::vec3(0.0f));
                    l_Properties.EmissiveStrength = a_PropertiesNode["EmissiveStrength"].as<float>(0.0f);
                    l_Properties.AlphaCutoff = a_PropertiesNode["AlphaCutoff"].as<float>(0.5f);
                    l_Properties.AlphaMode = static_cast<MaterialAlphaMode>(a_PropertiesNode["AlphaMode"].as<uint32_t>(0));

                    auto LoadTextureSlot = [](const YAML::Node& node, MaterialTextureSlot& slot)
                    {
                        if (!node)
                        {
                            return;
                        }

                        const AssetHandle l_UUID = node["TextureAssetUUID"].as<uint64_t>(InvalidAsset);
                        const std::string l_SourcePath = node["TextureSourcePath"].as<std::string>("");
                        slot.Enabled = node["Enabled"].as<bool>(false);

                        AssetHandle l_FinalHandle = l_UUID;
                        if (!l_SourcePath.empty())
                        {
                            AssetHandle l_ImportedHandle = AssetRegistry::Get().ImportAsset(l_SourcePath);
                            if (l_ImportedHandle != InvalidAsset)
                            {
                                l_FinalHandle = l_ImportedHandle;
                            }
                        }

                        slot.TextureAssetUUID = l_FinalHandle;
                        if (l_FinalHandle != InvalidAsset)
                        {
                            slot.TextureData = AssetRegistry::Get().LoadTexture(l_FinalHandle);
                        }
                    };

                    LoadTextureSlot(a_PropertiesNode["AlbedoTexture"], l_Properties.AlbedoTexture);
                    LoadTextureSlot(a_PropertiesNode["NormalTexture"], l_Properties.NormalTexture);
                    LoadTextureSlot(a_PropertiesNode["MetallicRoughnessTexture"], l_Properties.MetallicRoughnessTexture);
                    LoadTextureSlot(a_PropertiesNode["AmbientOcclusionTexture"], l_Properties.AmbientOcclusionTexture);
                    LoadTextureSlot(a_PropertiesNode["EmissiveTexture"], l_Properties.EmissiveTexture);
                }

                a_MaterialComponent.OverrideProperties = l_Properties;
                if (a_MaterialComponent.MaterialData)
                {
                    a_MaterialComponent.MaterialData->GetProperties() = l_Properties;
                }

                TR_CORE_TRACE("SceneSerializer: deserialised MaterialComponent (uuid={}, override={})", a_MaterialComponent.MaterialAssetUUID, a_MaterialComponent.UseOverrideProperties);
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
