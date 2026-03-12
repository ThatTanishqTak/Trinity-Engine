#pragma once

#include "Trinity/Assets/AssetHandle.h"
#include "Trinity/Assets/MeshAsset.h"

namespace Trinity
{
    namespace BuiltIn
    {
        static constexpr AssetUUID TriangleMeshUUID = 1;
        static constexpr AssetUUID QuadMeshUUID = 2;
        static constexpr AssetUUID CubeMeshUUID = 3;

        void RegisterAll();

        AssetHandle<MeshAsset> Triangle();
        AssetHandle<MeshAsset> Quad();
        AssetHandle<MeshAsset> Cube();
    }
}