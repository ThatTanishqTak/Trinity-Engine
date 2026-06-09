#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <Trinity/Core/UUID.h>
#include <Trinity/Assets/AssetMetadata.h>
#include <Trinity/Renderer/RHI/GraphicsDevice.h>

namespace Trinity
{
    class FileSystem;
    class MeshLibrary;
    class Mesh;
    class Material;
    class MaterialInstance;
    class TextureManager;

    class AssetDatabase
    {
    public:
        static constexpr uint64_t BuiltinCube = 1;

        AssetDatabase(FileSystem& fileSystem, MeshLibrary& meshLibrary, TextureManager& textureManager);

        AssetDatabase(const AssetDatabase&) = delete;
        AssetDatabase& operator=(const AssetDatabase&) = delete;

        void Initialize();
        void Refresh();

        const AssetMetadata* GetMetadata(UUID ID) const;
        UUID GetAssetByPath(const std::string& sourcePath) const;
        std::vector<UUID> GetAssetsOfType(AssetType type) const;

        std::shared_ptr<Mesh> ResolveMesh(UUID ID);
        std::shared_ptr<Material> ResolveMaterial(UUID ID);
        std::shared_ptr<MaterialInstance> ResolveMaterialInstance(UUID ID);
        TextureHandle ResolveTexture(UUID ID);

        const std::unordered_map<UUID, AssetMetadata>& GetAssets() const
        {
            return m_Assets;
        }
        const std::vector<UUID>& GetModified() const
        {
            return m_Modified;
        }

    private:
        void ScanDirectory();
        UUID RegisterAsset(const std::filesystem::path& sourceFile);
        std::shared_ptr<Material> GetDefaultMaterial();

    private:
        FileSystem& m_FileSystem;
        MeshLibrary& m_MeshLibrary;
        TextureManager& m_TextureManager;
        std::filesystem::path m_AssetsRoot;
        std::unordered_map<UUID, AssetMetadata> m_Assets;
        std::unordered_map<std::string, UUID> m_PathToID;
        std::vector<UUID> m_Modified;
        std::unordered_map<UUID, std::shared_ptr<Material>> m_MaterialCache;
        std::unordered_map<UUID, std::shared_ptr<MaterialInstance>> m_MaterialInstanceCache;
        std::shared_ptr<Material> m_DefaultMaterial;
    };
}