#pragma once

#include <cstdint>
#include <vector>

#include <Trinity/Renderer/RHI/Handle.h>
#include <Trinity/Renderer/Meshes/MeshData.h>

namespace Trinity
{
    class GraphicsDevice;

    class Mesh
    {
    public:
        explicit Mesh(GraphicsDevice& device);
        ~Mesh();

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        bool Upload(const MeshData& data);
        void Shutdown();

        bool IsValid() const { return m_VertexBuffer.IsValid() && m_IndexBuffer.IsValid(); }

        BufferHandle GetVertexBuffer() const { return m_VertexBuffer; }
        BufferHandle GetIndexBuffer() const { return m_IndexBuffer; }
        uint32_t GetVertexCount() const { return m_VertexCount; }
        uint32_t GetIndexCount() const { return m_IndexCount; }
        const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }
        const std::vector<MaterialSlot>& GetMaterialSlots() const { return m_MaterialSlots; }

    private:
        GraphicsDevice& m_Device;

        BufferHandle m_VertexBuffer;
        BufferHandle m_IndexBuffer;
        uint32_t m_VertexCount = 0;
        uint32_t m_IndexCount = 0;

        std::vector<Submesh> m_Submeshes;
        std::vector<MaterialSlot> m_MaterialSlots;
    };
}