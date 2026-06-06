#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Trinity/Core/UUID.h>
#include <Trinity/Assets/AssetType.h>

namespace Trinity
{
    struct AssetImportSettings
    {
        bool SRGB = true;
        bool GenerateMips = true;
    };

    struct AssetMetadata
    {
        UUID ID = UUID(0);
        AssetType Type = AssetType::None;
        std::string SourcePath;
        uint64_t SourceTimestamp = 0;
        AssetImportSettings Import;
        std::vector<UUID> Dependencies;

        bool IsValid() const { return Type != AssetType::None && static_cast<uint64_t>(ID) != 0; }
    };
}