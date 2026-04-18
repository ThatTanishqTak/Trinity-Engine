#pragma once

#include <cstdint>
#include <string>

namespace Trinity
{
    enum class AssetType : uint8_t
    {
        None = 0,
        Mesh,
        Texture,
        Scene
    };

    struct AssetMetadata
    {
        uint64_t   UUID       = 0;
        AssetType  Type       = AssetType::None;
        std::string SourcePath;
    };
}
