#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <Trinity/Renderer/Meshes/MeshImporter.h>

namespace Trinity
{
    class GraphicsDevice;
    class FileSystem;
    class Mesh;

    class MeshLibrary
    {
    public:
        MeshLibrary(GraphicsDevice& device, FileSystem& fileSystem);
        ~MeshLibrary();

        MeshLibrary(const MeshLibrary&) = delete;
        MeshLibrary& operator=(const MeshLibrary&) = delete;

        bool Initialize();
        void Shutdown();

        std::shared_ptr<Mesh> Load(const std::string& relativePath);
        std::shared_ptr<Mesh> GetCube();
        std::shared_ptr<Mesh> GetPlane();

        void Invalidate(const std::string& relativePath);

    private:
        std::shared_ptr<Mesh> CreateFromData(const MeshData& data, const std::string& debugName);

    private:
        GraphicsDevice& m_Device;
        FileSystem& m_FileSystem;
        MeshImporter m_Importer;

        std::unordered_map<std::string, std::shared_ptr<Mesh>> m_Cache;
        std::shared_ptr<Mesh> m_Cube;
        std::shared_ptr<Mesh> m_Plane;
    };
}