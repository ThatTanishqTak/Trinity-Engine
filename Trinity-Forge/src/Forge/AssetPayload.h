#pragma once

#include "Trinity/Asset/AssetMetadata.h"

struct AssetPayload
{
    char Path[512];
    Trinity::AssetType Type;
};

inline constexpr const char* AssetPayloadID = "ASSET_PAYLOAD";
