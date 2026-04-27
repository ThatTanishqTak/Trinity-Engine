#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/Renderer.h"

namespace Trinity
{
    std::shared_ptr<Mesh> Mesh::Create(const std::vector<Geometry::Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        auto a_Mesh = std::make_shared<Mesh>();
        a_Mesh->m_VertexCount = static_cast<uint32_t>(vertices.size());
        a_Mesh->m_IndexCount = static_cast<uint32_t>(indices.size());

        BufferSpecification l_VertexBufferSpecification{};
        l_VertexBufferSpecification.Size = vertices.size() * sizeof(Geometry::Vertex);
        l_VertexBufferSpecification.Usage = BufferUsage::VertexBuffer;
        l_VertexBufferSpecification.MemoryType = BufferMemoryType::CPUToGPU;
        l_VertexBufferSpecification.DebugName = "MeshVertexBuffer";

        a_Mesh->m_VertexBuffer = Renderer::GetAPI().CreateBuffer(l_VertexBufferSpecification);
        a_Mesh->m_VertexBuffer->SetData(vertices.data(), l_VertexBufferSpecification.Size);

        BufferSpecification l_IndexBufferSpecification{};
        l_IndexBufferSpecification.Size = indices.size() * sizeof(uint32_t);
        l_IndexBufferSpecification.Usage = BufferUsage::IndexBuffer;
        l_IndexBufferSpecification.MemoryType = BufferMemoryType::CPUToGPU;
        l_IndexBufferSpecification.DebugName = "MeshIndexBuffer";

        a_Mesh->m_IndexBuffer = Renderer::GetAPI().CreateBuffer(l_IndexBufferSpecification);
        a_Mesh->m_IndexBuffer->SetData(indices.data(), l_IndexBufferSpecification.Size);

        return a_Mesh;
    }
}