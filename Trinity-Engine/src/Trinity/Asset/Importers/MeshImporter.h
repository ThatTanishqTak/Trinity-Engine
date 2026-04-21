#pragma once

#include "Trinity/Renderer/Mesh.h"

#include <filesystem>
#include <memory>

namespace Trinity
{
    class MeshImporter
    {
    public:
        static std::shared_ptr<Mesh> Import(const std::filesystem::path& path);
    };
}