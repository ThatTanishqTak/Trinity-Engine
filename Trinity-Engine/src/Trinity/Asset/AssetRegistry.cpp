#include "Trinity/Asset/AssetRegistry.h"

#include "Trinity/Asset/Importers/MeshImporter.h"
#include "Trinity/Utilities/Log.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <random>

namespace Trinity
{
    AssetRegistry& AssetRegistry::Get()
    {
        static AssetRegistry s_Instance;
        return s_Instance;
    }

    AssetHandle AssetRegistry::GenerateUUID() const
    {
        static std::random_device s_RandomDevice;
        static std::mt19937_64 s_Engine(s_RandomDevice());
        static std::uniform_int_distribution<uint64_t> s_Distribution(1, UINT64_MAX);

        return s_Distribution(s_Engine);
    }

    AssetHandle AssetRegistry::ImportAsset(const std::filesystem::path& path)
    {
        const std::string l_Extension = path.extension().string();

        AssetType l_Type = AssetType::None;

        if (l_Extension == ".fbx" || l_Extension == ".obj" || l_Extension == ".gltf"
            || l_Extension == ".glb" || l_Extension == ".dae")
        {
            l_Type = AssetType::Mesh;
        }
        else if (l_Extension == ".png" || l_Extension == ".jpg"
                 || l_Extension == ".jpeg" || l_Extension == ".tga")
        {
            l_Type = AssetType::Texture;
        }
        else if (l_Extension == ".trinity")
        {
            l_Type = AssetType::Scene;
        }

        if (l_Type == AssetType::None)
        {
            TR_CORE_WARN("AssetRegistry: unrecognised file type '{}'", path.string());
            return InvalidAsset;
        }

        return ReadOrCreateMeta(path, l_Type);
    }

    const AssetMetadata* AssetRegistry::GetMetadata(AssetHandle handle) const
    {
        auto it = m_Metadata.find(handle);
        return (it != m_Metadata.end()) ? &it->second : nullptr;
    }

    std::shared_ptr<Mesh> AssetRegistry::LoadMesh(AssetHandle handle)
    {
        if (handle == InvalidAsset)
        {
            return nullptr;
        }

        auto l_CacheIt = m_MeshCache.find(handle);
        if (l_CacheIt != m_MeshCache.end())
        {
            return l_CacheIt->second;
        }

        const AssetMetadata* l_Meta = GetMetadata(handle);
        if (!l_Meta || l_Meta->Type != AssetType::Mesh)
        {
            TR_CORE_ERROR("AssetRegistry: no mesh metadata for handle {}", handle);
            return nullptr;
        }

        auto l_Mesh = MeshImporter::Import(l_Meta->SourcePath);
        if (l_Mesh)
        {
            m_MeshCache[handle] = l_Mesh;
        }

        return l_Mesh;
    }

    void AssetRegistry::Clear()
    {
        m_Metadata.clear();
        m_MeshCache.clear();
    }

    AssetHandle AssetRegistry::ReadOrCreateMeta(const std::filesystem::path& sourcePath, AssetType type)
    {
        const std::filesystem::path l_MetaPath = std::filesystem::path(sourcePath.string() + ".meta");

        if (std::filesystem::exists(l_MetaPath))
        {
            try
            {
                const YAML::Node l_Node = YAML::LoadFile(l_MetaPath.string());
                const AssetHandle l_Handle = l_Node["UUID"].as<uint64_t>();
                if (l_Handle != InvalidAsset)
                {
                    if (m_Metadata.find(l_Handle) == m_Metadata.end())
                    {
                        AssetMetadata l_Meta{};
                        l_Meta.UUID       = l_Handle;
                        l_Meta.Type       = type;
                        l_Meta.SourcePath = sourcePath.string();
                        m_Metadata[l_Handle] = l_Meta;
                    }
                    return l_Handle;
                }
            }
            catch (const YAML::Exception& e)
            {
                TR_CORE_WARN("AssetRegistry: failed to read meta '{}': {}", l_MetaPath.string(), e.what());
            }
        }

        const AssetHandle l_Handle = GenerateUUID();

        AssetMetadata l_Meta{};
        l_Meta.UUID       = l_Handle;
        l_Meta.Type       = type;
        l_Meta.SourcePath = sourcePath.string();
        m_Metadata[l_Handle] = l_Meta;

        YAML::Emitter l_Out;
        l_Out << YAML::BeginMap;
        l_Out << YAML::Key << "UUID" << YAML::Value << l_Handle;
        l_Out << YAML::Key << "Type" << YAML::Value << static_cast<int>(type);
        l_Out << YAML::EndMap;

        std::ofstream l_File(l_MetaPath);
        if (l_File.is_open())
        {
            l_File << l_Out.c_str();
        }
        else
        {
            TR_CORE_WARN("AssetRegistry: could not write meta '{}'", l_MetaPath.string());
        }

        return l_Handle;
    }
}
