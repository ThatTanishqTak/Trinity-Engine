#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <Trinity/Core/UUID.h>
#include <Trinity/Assets/AssetMetadata.h>

namespace Trinity
{
    class FileSystem;
    class MeshLibrary;
    class Mesh;

    class AssetDatabase
    {
    public:
        static constexpr uint64_t BuiltinCube = 1;

        AssetDatabase(FileSystem& fileSystem, MeshLibrary& meshLibrary);

        AssetDatabase(const AssetDatabase&) = delete;
        AssetDatabase& operator=(const AssetDatabase&) = delete;

        void Initialize();
        void Refresh();

        const AssetMetadata* GetMetadata(UUID ID) const;
        UUID GetAssetByPath(const std::string& sourcePath) const;
        std::vector<UUID> GetAssetsOfType(AssetType type) const;

        std::shared_ptr<Mesh> ResolveMesh(UUID ID);

        const std::unordered_map<UUID, AssetMetadata>& GetAssets() const { return m_Assets; }
        const std::vector<UUID>& GetModified() const { return m_Modified; }

    private:
        void ScanDirectory();
        UUID RegisterAsset(const std::filesystem::path& sourceFile);

    private:
        FileSystem& m_FileSystem;
        MeshLibrary& m_MeshLibrary;
        std::filesystem::path m_AssetsRoot;
        std::unordered_map<UUID, AssetMetadata> m_Assets;
        std::unordered_map<std::string, UUID> m_PathToID;
        std::vector<UUID> m_Modified;
    };
}