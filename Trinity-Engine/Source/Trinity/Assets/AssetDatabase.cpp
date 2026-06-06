#include <Trinity/Assets/AssetDatabase.h>

#include <optional>
#include <system_error>
#include <utility>

#include <Trinity/Assets/AssetMetaFile.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Renderer/Meshes/MeshLibrary.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    static uint64_t FileTimestamp(const std::filesystem::path& path)
    {
        std::error_code l_Error;
        std::filesystem::file_time_type l_Time = std::filesystem::last_write_time(path, l_Error);
        if (l_Error)
        {
            return 0;
        }

        return static_cast<uint64_t>(l_Time.time_since_epoch().count());
    }

    AssetDatabase::AssetDatabase(FileSystem& fileSystem, MeshLibrary& meshLibrary) : m_FileSystem(fileSystem), m_MeshLibrary(meshLibrary)
    {

    }

    void AssetDatabase::Initialize()
    {
        m_AssetsRoot = m_FileSystem.Resolve(BaseDirectory::Executable, "Assets");
        Refresh();
    }

    void AssetDatabase::Refresh()
    {
        m_Modified.clear();

        std::error_code l_Error;
        if (!std::filesystem::exists(m_AssetsRoot, l_Error))
        {
            TR_CORE_WARN("AssetDatabase: assets root '{}' not found", m_AssetsRoot.string());

            return;
        }

        ScanDirectory();

        TR_CORE_INFO("AssetDatabase: {} assets tracked ({} modified)", m_Assets.size(), m_Modified.size());
    }

    void AssetDatabase::ScanDirectory()
    {
        std::error_code l_Error;
        for (const std::filesystem::directory_entry& it_Entry : std::filesystem::recursive_directory_iterator(m_AssetsRoot, l_Error))
        {
            if (!it_Entry.is_regular_file())
            {
                continue;
            }

            const std::filesystem::path& l_Path = it_Entry.path();
            if (l_Path.extension() == ".meta")
            {
                continue;
            }

            if (AssetTypeFromExtension(l_Path.extension().string()) == AssetType::None)
            {
                continue;
            }

            RegisterAsset(l_Path);
        }
    }

    UUID AssetDatabase::RegisterAsset(const std::filesystem::path& sourceFile)
    {
        std::filesystem::path l_MetaPath = sourceFile;
        l_MetaPath += ".meta";

        std::error_code l_Error;
        std::filesystem::path l_Relative = std::filesystem::relative(sourceFile, m_AssetsRoot, l_Error);
        std::string l_SourcePath = "Assets/" + l_Relative.generic_string();

        AssetType l_Type = AssetTypeFromExtension(sourceFile.extension().string());
        uint64_t l_Timestamp = FileTimestamp(sourceFile);

        std::optional<AssetMetadata> l_Existing = std::filesystem::exists(l_MetaPath) ? AssetMetaFile::Read(l_MetaPath) : std::nullopt;

        AssetMetadata l_Metadata;
        if (l_Existing.has_value() && l_Existing->IsValid())
        {
            l_Metadata = l_Existing.value();
            l_Metadata.SourcePath = l_SourcePath;
            l_Metadata.Type = l_Type;

            if (l_Metadata.SourceTimestamp != l_Timestamp)
            {
                l_Metadata.SourceTimestamp = l_Timestamp;

                AssetMetaFile::Write(l_MetaPath, l_Metadata);
                m_Modified.push_back(l_Metadata.ID);

                if (l_Metadata.Type == AssetType::Mesh)
                {
                    m_MeshLibrary.Invalidate(l_Metadata.SourcePath);
                }
            }
        }
        else
        {
            l_Metadata.ID = UUID();
            l_Metadata.Type = l_Type;
            l_Metadata.SourcePath = l_SourcePath;
            l_Metadata.SourceTimestamp = l_Timestamp;
            AssetMetaFile::Write(l_MetaPath, l_Metadata);
        }

        m_Assets[l_Metadata.ID] = l_Metadata;
        m_PathToID[l_Metadata.SourcePath] = l_Metadata.ID;

        return l_Metadata.ID;
    }

    const AssetMetadata* AssetDatabase::GetMetadata(UUID id) const
    {
        auto it_Found = m_Assets.find(id);

        return it_Found != m_Assets.end() ? &it_Found->second : nullptr;
    }

    UUID AssetDatabase::GetAssetByPath(const std::string& sourcePath) const
    {
        auto it_Found = m_PathToID.find(sourcePath);

        return it_Found != m_PathToID.end() ? it_Found->second : UUID(0);
    }

    std::vector<UUID> AssetDatabase::GetAssetsOfType(AssetType type) const
    {
        std::vector<UUID> l_Result;
        for (const std::pair<const UUID, AssetMetadata>& it_Pair : m_Assets)
        {
            if (it_Pair.second.Type == type)
            {
                l_Result.push_back(it_Pair.first);
            }
        }

        return l_Result;
    }

    std::shared_ptr<Mesh> AssetDatabase::ResolveMesh(UUID id)
    {
        uint64_t l_Raw = static_cast<uint64_t>(id);
        if (l_Raw == 0)
        {
            return nullptr;
        }

        if (l_Raw == BuiltinCube)
        {
            return m_MeshLibrary.GetCube();
        }

        const AssetMetadata* l_Metadata = GetMetadata(id);
        if (l_Metadata == nullptr || l_Metadata->Type != AssetType::Mesh)
        {
            TR_CORE_WARN("AssetDatabase: mesh {} unresolved, using cube fallback", l_Raw);

            return m_MeshLibrary.GetCube();
        }

        return m_MeshLibrary.Load(l_Metadata->SourcePath);
    }
}