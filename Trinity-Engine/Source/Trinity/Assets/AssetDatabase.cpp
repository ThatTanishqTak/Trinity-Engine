#include <Trinity/Assets/AssetDatabase.h>

#include <optional>
#include <system_error>
#include <utility>

#include <Trinity/Assets/AssetMetaFile.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Renderer/Meshes/MeshLibrary.h>
#include <Trinity/Renderer/Materials/Material.h>
#include <Trinity/Renderer/Materials/MaterialInstance.h>
#include <Trinity/Renderer/Textures/TextureManager.h>
#include <Trinity/Serialization/MaterialSerializer.h>
#include <Trinity/Audio/Frontend/AudioEngine.h>
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

    AssetDatabase::AssetDatabase(FileSystem& fileSystem, MeshLibrary& meshLibrary, TextureManager& textureManager, AudioEngine& audioEngine) : m_FileSystem(fileSystem), m_MeshLibrary(meshLibrary), m_TextureManager(textureManager), m_AudioEngine(audioEngine)
    {

    }

    void AssetDatabase::Initialize()
    {
        ("INITIALIZING ASSET DATABASE");

        m_AssetsRoot = m_FileSystem.Resolve(BaseDirectory::Executable, "Assets");
        Refresh();

        ("Root directory: {}", m_AssetsRoot.string());

        ("ASSET DATABASE INITIALIZED");
    }

    void AssetDatabase::Refresh()
    {
        m_Modified.clear();
        m_MaterialCache.clear();
        m_MaterialInstanceCache.clear();
        m_AudioClipCache.clear();

        std::error_code l_Error;
        if (!std::filesystem::exists(m_AssetsRoot, l_Error))
        {
            ("AssetDatabase: assets root '{}' not found", m_AssetsRoot.string());

            return;
        }

        ScanDirectory();

        ("AssetDatabase: {} assets tracked ({} modified)", m_Assets.size(), m_Modified.size());
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

        if (l_Raw == BuiltinPlane)
        {
            return m_MeshLibrary.GetPlane();
        }

        const AssetMetadata* l_Metadata = GetMetadata(id);
        if (l_Metadata == nullptr || l_Metadata->Type != AssetType::Mesh)
        {
            ("AssetDatabase: mesh {} unresolved, using cube fallback", l_Raw);

            return m_MeshLibrary.GetCube();
        }

        return m_MeshLibrary.Load(l_Metadata->SourcePath);
    }

    std::shared_ptr<Material> AssetDatabase::GetDefaultMaterial()
    {
        if (m_DefaultMaterial == nullptr)
        {
            m_DefaultMaterial = std::make_shared<Material>();
        }

        return m_DefaultMaterial;
    }

    std::shared_ptr<Material> AssetDatabase::ResolveMaterial(UUID id)
    {
        uint64_t l_Raw = static_cast<uint64_t>(id);
        if (l_Raw == 0)
        {
            return GetDefaultMaterial();
        }

        auto it_Cached = m_MaterialCache.find(id);
        if (it_Cached != m_MaterialCache.end())
        {
            return it_Cached->second;
        }

        const AssetMetadata* l_Metadata = GetMetadata(id);
        if (l_Metadata == nullptr)
        {
            ("AssetDatabase: material {} unresolved, using default", l_Raw);

            return GetDefaultMaterial();
        }

        if (l_Metadata->Type == AssetType::Material)
        {
            std::filesystem::path l_Path = m_FileSystem.Resolve(BaseDirectory::Executable, l_Metadata->SourcePath);
            std::optional<Material> l_Loaded = MaterialSerializer::DeserializeMaterial(l_Path);
            if (!l_Loaded.has_value())
            {
                return GetDefaultMaterial();
            }

            std::shared_ptr<Material> l_Material = std::make_shared<Material>(std::move(l_Loaded.value()));
            m_MaterialCache[id] = l_Material;

            return l_Material;
        }

        if (l_Metadata->Type == AssetType::MaterialInstance)
        {
            std::shared_ptr<MaterialInstance> l_Instance = ResolveMaterialInstance(id);
            if (l_Instance == nullptr)
            {
                return GetDefaultMaterial();
            }

            std::shared_ptr<Material> l_Base = ResolveMaterial(l_Instance->GetParent());
            std::shared_ptr<Material> l_Flattened = std::make_shared<Material>(*l_Base);
            l_Instance->ApplyOverrides(*l_Flattened);
            m_MaterialCache[id] = l_Flattened;

            return l_Flattened;
        }

        ("AssetDatabase: asset {} is not a material, using default", l_Raw);

        return GetDefaultMaterial();
    }

    std::shared_ptr<MaterialInstance> AssetDatabase::ResolveMaterialInstance(UUID id)
    {
        uint64_t l_Raw = static_cast<uint64_t>(id);
        if (l_Raw == 0)
        {
            return nullptr;
        }

        auto it_Cached = m_MaterialInstanceCache.find(id);
        if (it_Cached != m_MaterialInstanceCache.end())
        {
            return it_Cached->second;
        }

        const AssetMetadata* l_Metadata = GetMetadata(id);
        if (l_Metadata == nullptr || l_Metadata->Type != AssetType::MaterialInstance)
        {
            ("AssetDatabase: material instance {} unresolved", l_Raw);

            return nullptr;
        }

        std::filesystem::path l_Path = m_FileSystem.Resolve(BaseDirectory::Executable, l_Metadata->SourcePath);
        std::optional<MaterialInstance> l_Loaded = MaterialSerializer::DeserializeMaterialInstance(l_Path);
        if (!l_Loaded.has_value())
        {
            return nullptr;
        }

        std::shared_ptr<MaterialInstance> l_Instance = std::make_shared<MaterialInstance>(std::move(l_Loaded.value()));
        m_MaterialInstanceCache[id] = l_Instance;

        return l_Instance;
    }

    TextureHandle AssetDatabase::ResolveTexture(UUID id)
    {
        uint64_t l_Raw = static_cast<uint64_t>(id);
        if (l_Raw == 0)
        {
            return m_TextureManager.White();
        }

        const AssetMetadata* l_Metadata = GetMetadata(id);
        if (l_Metadata == nullptr || l_Metadata->Type != AssetType::Texture)
        {
            ("AssetDatabase: texture {} unresolved, using error texture", l_Raw);

            return m_TextureManager.Error();
        }

        return m_TextureManager.Load(l_Metadata->SourcePath, l_Metadata->Import.SRGB, true);
    }

    AudioClipHandle AssetDatabase::ResolveAudioClip(UUID id)
    {
        uint64_t l_Raw = static_cast<uint64_t>(id);
        if (l_Raw == 0)
        {
            return AudioClipHandle::Invalid;
        }

        auto it_Cached = m_AudioClipCache.find(id);
        if (it_Cached != m_AudioClipCache.end())
        {
            return it_Cached->second;
        }

        const AssetMetadata* l_Metadata = GetMetadata(id);
        if (l_Metadata == nullptr || l_Metadata->Type != AssetType::Audio)
        {
            ("AssetDatabase: audio clip {} unresolved", l_Raw);

            return AudioClipHandle::Invalid;
        }

        std::filesystem::path l_Path = m_FileSystem.Resolve(BaseDirectory::Executable, l_Metadata->SourcePath);
        AudioClipHandle l_Clip = m_AudioEngine.LoadClip(l_Path);
        m_AudioClipCache[id] = l_Clip;

        return l_Clip;
    }
}