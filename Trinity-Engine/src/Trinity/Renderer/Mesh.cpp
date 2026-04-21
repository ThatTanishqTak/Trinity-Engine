#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/Renderer.h"

namespace Trinity
{
    std::shared_ptr<Mesh> Mesh::Create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        auto a_Mesh = std::make_shared<Mesh>();
        a_Mesh->m_VertexCount = static_cast<uint32_t>(vertices.size());
        a_Mesh->m_IndexCount = static_cast<uint32_t>(indices.size());

        BufferSpecification l_VertexBufferSpecification{};
        l_VertexBufferSpecification.Size = vertices.size() * sizeof(Vertex);
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

    namespace Primitives
    {
        std::shared_ptr<Mesh> CreateCube()
        {
            // 24 vertices — 4 per face, unique normals per face
            std::vector<Vertex> l_Vertices =
            {
                // +X
                { {  0.5f, -0.5f, -0.5f }, { 1, 0, 0 }, { 0, 0 } },
                { {  0.5f,  0.5f, -0.5f }, { 1, 0, 0 }, { 1, 0 } },
                { {  0.5f,  0.5f,  0.5f }, { 1, 0, 0 }, { 1, 1 } },
                { {  0.5f, -0.5f,  0.5f }, { 1, 0, 0 }, { 0, 1 } },
                // -X
                { { -0.5f, -0.5f,  0.5f }, { -1, 0, 0 }, { 0, 0 } },
                { { -0.5f,  0.5f,  0.5f }, { -1, 0, 0 }, { 1, 0 } },
                { { -0.5f,  0.5f, -0.5f }, { -1, 0, 0 }, { 1, 1 } },
                { { -0.5f, -0.5f, -0.5f }, { -1, 0, 0 }, { 0, 1 } },
                // +Y
                { { -0.5f,  0.5f, -0.5f }, { 0,  1, 0 }, { 0, 0 } },
                { { -0.5f,  0.5f,  0.5f }, { 0,  1, 0 }, { 0, 1 } },
                { {  0.5f,  0.5f,  0.5f }, { 0,  1, 0 }, { 1, 1 } },
                { {  0.5f,  0.5f, -0.5f }, { 0,  1, 0 }, { 1, 0 } },
                // -Y
                { { -0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 0, 0 } },
                { { -0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 0, 1 } },
                { {  0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 1, 1 } },
                { {  0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 1, 0 } },
                // +Z
                { { -0.5f, -0.5f,  0.5f }, { 0, 0, 1 }, { 0, 0 } },
                { {  0.5f, -0.5f,  0.5f }, { 0, 0, 1 }, { 1, 0 } },
                { {  0.5f,  0.5f,  0.5f }, { 0, 0, 1 }, { 1, 1 } },
                { { -0.5f,  0.5f,  0.5f }, { 0, 0, 1 }, { 0, 1 } },
                // -Z
                { {  0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 0, 0 } },
                { { -0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0 } },
                { { -0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 1, 1 } },
                { {  0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 0, 1 } },
            };

            std::vector<uint32_t> l_Indices;
            l_Indices.reserve(36);
            for (uint32_t face = 0; face < 6; face++)
            {
                const uint32_t l_Base = face * 4;
                l_Indices.insert(l_Indices.end(), { l_Base, l_Base + 1, l_Base + 2, l_Base, l_Base + 2, l_Base + 3 });
            }

            return Mesh::Create(l_Vertices, l_Indices);
        }
    }
}