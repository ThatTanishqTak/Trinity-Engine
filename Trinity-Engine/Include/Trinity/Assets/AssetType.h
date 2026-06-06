#pragma once

#include <cstdint>
#include <string>

namespace Trinity
{
    enum class AssetType : uint32_t
    {
        None = 0,
        Mesh,
        Texture
    };

    const char* AssetTypeToString(AssetType type);
    AssetType AssetTypeFromString(const std::string& text);
    AssetType AssetTypeFromExtension(const std::string& extension);
}