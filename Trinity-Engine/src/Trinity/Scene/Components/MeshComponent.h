#pragma once

#include "Trinity/Renderer/Mesh.h"

#include <memory>

namespace Trinity
{
    struct MeshComponent
    {
        std::shared_ptr<Mesh> MeshData;
    };
}
