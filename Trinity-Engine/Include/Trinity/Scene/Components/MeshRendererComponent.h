#pragma once

#include <memory>
#include <vector>

#include <Trinity/Core/UUID.h>

namespace Trinity
{
    class Mesh;

    struct MeshRendererComponent
    {
        std::shared_ptr<Mesh> MeshReference;
        UUID MeshAsset = UUID(0);
        std::vector<UUID> Materials;
    };
}