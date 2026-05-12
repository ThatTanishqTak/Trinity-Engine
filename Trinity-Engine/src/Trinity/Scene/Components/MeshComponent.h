#pragma once

#include "Trinity/Asset/AssetHandle.h"
#include "Trinity/Renderer/Mesh.h"

#include <memory>

namespace Trinity
{
    enum class BuiltinMeshType : uint32_t
    {
        None = 0,
        Triangle,
        Quad,
        Cube
    };

    struct MeshComponent
    {
        AssetHandle MeshAssetUUID = InvalidAsset;
        std::shared_ptr<Mesh> MeshData;

        BuiltinMeshType BuiltinType = BuiltinMeshType::None;
    };
}