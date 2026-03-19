#pragma once

#include "Trinity/Assets/AssetHandle.h"

#include <filesystem>
#include <string>
#include <vector>

namespace Trinity
{
    enum class AssetType : uint8_t
    {
        Unknown = 0,
        Scene,
        Mesh,
        Texture,
        Material,
        Script,
    };

    struct AssetMetadata
    {
        std::filesystem::path m_FilePath;
        std::string m_Name;
        AssetType m_Type = AssetType::Unknown;
        AssetUUID m_UUID = NullAssetUUID;
    };

    class AssetRegistry
    {
    public:
        static AssetRegistry& Get();

        void Scan(const std::filesystem::path& rootPath);
        void Refresh();

        const std::vector<AssetMetadata>& GetAll() const { return m_Assets; }
        std::vector<AssetMetadata> GetByType(AssetType type) const;
        const AssetMetadata* FindByPath(const std::filesystem::path& path) const;
        const AssetMetadata* FindByUUID(AssetUUID uuid) const;

        const std::filesystem::path& GetRootPath() const { return m_RootPath; }
        bool IsScanned() const { return m_Scanned; }

        static AssetType DetermineType(const std::string& extension);
        static std::string TypeToString(AssetType type);
        static const char* TypeToIcon(AssetType type);

    private:
        AssetRegistry() = default;
        void ScanDirectory(const std::filesystem::path& path);

    private:
        std::filesystem::path m_RootPath;
        std::vector<AssetMetadata> m_Assets;
        bool m_Scanned = false;
    };
}