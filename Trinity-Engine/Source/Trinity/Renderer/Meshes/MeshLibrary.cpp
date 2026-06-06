#include <Trinity/Renderer/Meshes/MeshLibrary.h>

#include <filesystem>

#include <glm/glm.hpp>

#include <Trinity/Renderer/Meshes/Mesh.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    static MeshData BuildCubeMeshData()
    {
        MeshData l_Data;

        const glm::vec3 l_Normals[6] = { { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } };
        const glm::vec3 l_Tangents[6] = { { 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } };
        const glm::vec3 l_FaceVertices[6][4] =
        {
            { { -0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f } },
            { {  0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f,  0.5f } },
            { { -0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f, -0.5f } },
            { { -0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f } },
            { { -0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f,  0.5f }, { -0.5f, -0.5f,  0.5f } }
        };
        const glm::vec2 l_UVs[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

        for (int l_Face = 0; l_Face < 6; ++l_Face)
        {
            for (int l_Corner = 0; l_Corner < 4; ++l_Corner)
            {
                MeshVertex l_Vertex{};
                l_Vertex.Position = l_FaceVertices[l_Face][l_Corner];
                l_Vertex.Normal = l_Normals[l_Face];
                l_Vertex.Tangent = l_Tangents[l_Face];
                l_Vertex.UV = l_UVs[l_Corner];
                l_Data.Vertices.push_back(l_Vertex);
            }

            uint32_t l_Base = static_cast<uint32_t>(l_Face * 4);
            l_Data.Indices.push_back(l_Base + 0);
            l_Data.Indices.push_back(l_Base + 1);
            l_Data.Indices.push_back(l_Base + 2);
            l_Data.Indices.push_back(l_Base + 2);
            l_Data.Indices.push_back(l_Base + 3);
            l_Data.Indices.push_back(l_Base + 0);
        }

        Submesh l_Submesh;
        l_Submesh.IndexCount = static_cast<uint32_t>(l_Data.Indices.size());
        l_Submesh.Name = "Cube";
        l_Data.Submeshes.push_back(l_Submesh);

        MaterialSlot l_Slot;
        l_Slot.Name = "Default";
        l_Data.MaterialSlots.push_back(l_Slot);

        l_Data.Diagnostics.SourcePath = "<procedural cube>";
        l_Data.Diagnostics.SourceFormat = "procedural";

        return l_Data;
    }

    MeshLibrary::MeshLibrary(GraphicsDevice& device, FileSystem& fileSystem) : m_Device(device), m_FileSystem(fileSystem)
    {

    }

    MeshLibrary::~MeshLibrary() = default;

    bool MeshLibrary::Initialize()
    {
        if (!GetCube())
        {
            TR_CORE_ERROR("MeshLibrary: failed to create procedural cube");

            return false;
        }

        return true;
    }

    void MeshLibrary::Shutdown()
    {
        m_Cache.clear();
        m_Cube.reset();
    }

    std::shared_ptr<Mesh> MeshLibrary::Load(const std::string& relativePath)
    {
        auto it_Cached = m_Cache.find(relativePath);
        if (it_Cached != m_Cache.end())
        {
            return it_Cached->second;
        }

        std::filesystem::path l_FullPath = m_FileSystem.Resolve(BaseDirectory::Executable, relativePath);
        if (!std::filesystem::exists(l_FullPath))
        {
            TR_CORE_WARN("MeshLibrary: '{}' not found, using procedural cube", relativePath);
            m_Cache[relativePath] = GetCube();

            return m_Cache[relativePath];
        }

        std::optional<MeshData> l_Result = m_Importer.Import(l_FullPath);
        if (!l_Result.has_value())
        {
            TR_CORE_ERROR("MeshLibrary: import failed for '{}', using procedural cube", relativePath);
            m_Cache[relativePath] = GetCube();

            return m_Cache[relativePath];
        }

        std::shared_ptr<Mesh> l_Mesh = CreateFromData(l_Result.value(), relativePath);
        if (!l_Mesh)
        {
            m_Cache[relativePath] = GetCube();

            return m_Cache[relativePath];
        }

        m_Cache[relativePath] = l_Mesh;

        return l_Mesh;
    }

    std::shared_ptr<Mesh> MeshLibrary::GetCube()
    {
        if (m_Cube)
        {
            return m_Cube;
        }

        m_Cube = CreateFromData(BuildCubeMeshData(), "ProceduralCube");

        return m_Cube;
    }

    void MeshLibrary::Invalidate(const std::string& relativePath)
    {
        m_Cache.erase(relativePath);
    }

    std::shared_ptr<Mesh> MeshLibrary::CreateFromData(const MeshData& data, const std::string& debugName)
    {
        std::shared_ptr<Mesh> l_Mesh = std::make_shared<Mesh>(m_Device);
        if (!l_Mesh->Upload(data))
        {
            TR_CORE_ERROR("MeshLibrary: upload failed for '{}'", debugName);

            return nullptr;
        }

        return l_Mesh;
    }
}