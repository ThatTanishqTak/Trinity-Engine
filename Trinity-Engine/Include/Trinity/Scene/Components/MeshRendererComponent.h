#pragma once

#include <memory>

namespace Trinity
{
    class Mesh;

    struct MeshRendererComponent
    {
        std::shared_ptr<Mesh> MeshReference;
    };
}