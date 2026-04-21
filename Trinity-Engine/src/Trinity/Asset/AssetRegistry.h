#pragma once

#include "Trinity/Asset/AssetHandle.h"
#include "Trinity/Asset/AssetMetadata.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace Trinity
{
    class AssetRegistry
    {
    public:
        static AssetRegistry& Get();

        AssetHandle ImportAsset(const std::filesystem::path& path);
        AssetHandle RegisterMesh(std::shared_ptr<Mesh> mesh);

        const AssetMetadata* GetMetadata(AssetHandle handle) const;

        std::shared_ptr<Mesh> LoadMesh(AssetHandle handle);
        std::shared_ptr<Texture> LoadTexture(AssetHandle handle);

        void Clear();

    private:
        AssetRegistry() = default;

        AssetHandle GenerateUUID() const;

        AssetHandle ReadOrCreateMeta(const std::filesystem::path& sourcePath, AssetType type);

        std::unordered_map<AssetHandle, AssetMetadata> m_Metadata;
        std::unordered_map<AssetHandle, std::shared_ptr<Mesh>> m_MeshCache;
        std::unordered_map<AssetHandle, std::shared_ptr<Texture>> m_TextureCache;
    };
}