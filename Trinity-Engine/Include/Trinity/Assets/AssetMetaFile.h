#pragma once

#include <filesystem>
#include <optional>

#include <Trinity/Assets/AssetMetadata.h>

namespace Trinity
{
    class AssetMetaFile
    {
    public:
        static bool Write(const std::filesystem::path& metaPath, const AssetMetadata& metadata);
        static std::optional<AssetMetadata> Read(const std::filesystem::path& metaPath);
    };
}