#pragma once

#include "Trinity/Asset/AssetHandle.h"
#include "Trinity/Renderer/Mesh.h"

#include <memory>

namespace Trinity
{
    struct MeshComponent
    {
        AssetHandle MeshAssetUUID = InvalidAsset;
        std::shared_ptr<Mesh> MeshData;
    };
}
