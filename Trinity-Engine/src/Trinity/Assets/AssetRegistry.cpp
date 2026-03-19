#include "Trinity/Assets/AssetRegistry.h"
#include "Trinity/Assets/AssetManager.h"
#include "Trinity/Utilities/Log.h"

namespace Trinity
{
    AssetRegistry& AssetRegistry::Get()
    {
        static AssetRegistry s_Instance;
        return s_Instance;
    }

    void AssetRegistry::Scan(const std::filesystem::path& rootPath)
    {
        m_RootPath = rootPath;
        m_Assets.clear();
        m_Scanned = false;

        if (!std::filesystem::exists(rootPath))
        {
            TR_CORE_WARN("AssetRegistry::Scan — path does not exist: {}", rootPath.string());
            return;
        }

        ScanDirectory(rootPath);
        m_Scanned = true;

        TR_CORE_INFO("AssetRegistry: scanned {} asset(s) from '{}'", m_Assets.size(), rootPath.string());
    }

    void AssetRegistry::Refresh()
    {
        if (!m_RootPath.empty())
        {
            Scan(m_RootPath);
        }
    }

    std::vector<AssetMetadata> AssetRegistry::GetByType(AssetType type) const
    {
        std::vector<AssetMetadata> l_Result;
        for (const AssetMetadata& it_Asset : m_Assets)
        {
            if (it_Asset.m_Type == type)
            {
                l_Result.push_back(it_Asset);
            }
        }

        return l_Result;
    }

    const AssetMetadata* AssetRegistry::FindByPath(const std::filesystem::path& path) const
    {
        for (const AssetMetadata& it_Asset : m_Assets)
        {
            if (it_Asset.m_FilePath == path)
            {
                return &it_Asset;
            }
        }

        return nullptr;
    }

    const AssetMetadata* AssetRegistry::FindByUUID(AssetUUID uuid) const
    {
        for (const AssetMetadata& it_Asset : m_Assets)
        {
            if (it_Asset.m_UUID == uuid)
            {
                return &it_Asset;
            }
        }

        return nullptr;
    }

    AssetType AssetRegistry::DetermineType(const std::string& extension)
    {
        if (extension == ".trinity")                                                              return AssetType::Scene;
        if (extension == ".fbx" || extension == ".obj" || extension == ".gltf" || extension == ".glb") return AssetType::Mesh;
        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".hdr" || extension == ".tga") return AssetType::Texture;
        if (extension == ".mat")                                                                  return AssetType::Material;
        if (extension == ".lua")                                                                  return AssetType::Script;

        return AssetType::Unknown;
    }

    std::string AssetRegistry::TypeToString(AssetType type)
    {
        switch (type)
        {
            case AssetType::Scene:    return "Scene";
            case AssetType::Mesh:     return "Mesh";
            case AssetType::Texture:  return "Texture";
            case AssetType::Material: return "Material";
            case AssetType::Script:   return "Script";
            default:                  return "Unknown";
        }
    }

    const char* AssetRegistry::TypeToIcon(AssetType type)
    {
        switch (type)
        {
            case AssetType::Scene:    return "[SCN]";
            case AssetType::Mesh:     return "[MSH]";
            case AssetType::Texture:  return "[TEX]";
            case AssetType::Material: return "[MAT]";
            case AssetType::Script:   return "[LUA]";
            default:                  return "[???]";
        }
    }

    void AssetRegistry::ScanDirectory(const std::filesystem::path& path)
    {
        std::error_code l_Error;
        for (const std::filesystem::directory_entry& it_Entry : std::filesystem::recursive_directory_iterator(path, l_Error))
        {
            if (!it_Entry.is_regular_file())
            {
                continue;
            }

            const std::filesystem::path& l_FilePath = it_Entry.path();
            const std::string l_Extension = l_FilePath.extension().string();
            const AssetType l_Type = DetermineType(l_Extension);

            if (l_Type == AssetType::Unknown)
            {
                continue;
            }

            AssetMetadata l_Metadata{};
            l_Metadata.m_FilePath = l_FilePath;
            l_Metadata.m_Name = l_FilePath.filename().string();
            l_Metadata.m_Type = l_Type;
            l_Metadata.m_UUID = AssetManager::GenerateUUID();

            m_Assets.push_back(std::move(l_Metadata));
        }
    }
}