#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <Trinity/Renderer/Meshes/MeshData.h>

namespace Trinity
{
    class MeshImporter
    {
    public:
        MeshImporter();
        ~MeshImporter();

        MeshImporter(const MeshImporter&) = delete;
        MeshImporter& operator=(const MeshImporter&) = delete;

        std::optional<MeshData> Import(const std::filesystem::path& path);

    private:
        struct Implementation;
        std::unique_ptr<Implementation> m_Implementation;
    };
}