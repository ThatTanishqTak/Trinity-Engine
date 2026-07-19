#include <Trinity/Renderer/Meshes/Mesh.h>

#include <Trinity/Renderer/RHI/GraphicsDevice.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    Mesh::Mesh(GraphicsDevice& device) : m_Device(device)
    {

    }

    Mesh::~Mesh()
    {
        Shutdown();
    }

    bool Mesh::Upload(const MeshData& data)
    {
        Shutdown();

        if (data.Vertices.empty() || data.Indices.empty())
        {

            return false;
        }

        BufferDescription l_VertexDescription;
        l_VertexDescription.Size = static_cast<uint64_t>(data.Vertices.size()) * sizeof(MeshVertex);
        l_VertexDescription.Usage = BufferUsage::Vertex;
        l_VertexDescription.Memory = MemoryUsage::GpuOnly;
        l_VertexDescription.InitialData = data.Vertices.data();
        l_VertexDescription.DebugName = "Mesh.Vertices";
        m_VertexBuffer = m_Device.CreateBuffer(l_VertexDescription);

        BufferDescription l_IndexDescription;
        l_IndexDescription.Size = static_cast<uint64_t>(data.Indices.size()) * sizeof(uint32_t);
        l_IndexDescription.Usage = BufferUsage::Index;
        l_IndexDescription.Memory = MemoryUsage::GpuOnly;
        l_IndexDescription.InitialData = data.Indices.data();
        l_IndexDescription.DebugName = "Mesh.Indices";
        m_IndexBuffer = m_Device.CreateBuffer(l_IndexDescription);

        if (!m_VertexBuffer.IsValid() || !m_IndexBuffer.IsValid())
        {

            Shutdown();

            return false;
        }

        m_VertexCount = static_cast<uint32_t>(data.Vertices.size());
        m_IndexCount = static_cast<uint32_t>(data.Indices.size());
        m_Submeshes = data.Submeshes;
        m_MaterialSlots = data.MaterialSlots;



        return true;
    }

    void Mesh::Shutdown()
    {
        if (m_IndexBuffer.IsValid())
        {
            m_Device.DestroyBuffer(m_IndexBuffer);
            m_IndexBuffer = BufferHandle{};
        }

        if (m_VertexBuffer.IsValid())
        {
            m_Device.DestroyBuffer(m_VertexBuffer);
            m_VertexBuffer = BufferHandle{};
        }

        m_Submeshes.clear();
        m_MaterialSlots.clear();
        m_VertexCount = 0;
        m_IndexCount = 0;
    }
}