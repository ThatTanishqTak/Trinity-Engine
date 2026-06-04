#pragma once

#include <memory>
#include <string>

namespace Trinity
{
    class Mesh;

    struct MeshRendererComponent
    {
        std::shared_ptr<Mesh> MeshReference;
        std::string MeshPath;
    };
}