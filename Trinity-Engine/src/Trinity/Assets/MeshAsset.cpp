#include "Trinity/Assets/MeshAsset.h"

#include "Trinity/Renderer/Buffer.h"
#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/VulkanAllocator.h"
#include "Trinity/Renderer/Vulkan/VulkanUploadContext.h"

namespace Trinity
{
    std::shared_ptr<MeshAsset> MeshAsset::CreateFromMeshData(const Geometry::MeshData& meshData, VulkanAllocator& allocator, VulkanUploadContext& uploadContext, const std::string& name)
    {
        auto l_Asset = std::make_shared<MeshAsset>();
        l_Asset->m_Name = name;
        l_Asset->m_VertexCount = static_cast<uint32_t>(meshData.Vertices.size());
        l_Asset->m_IndexCount = static_cast<uint32_t>(meshData.Indices.size());

        const uint64_t l_VertexDataSize = meshData.Vertices.size() * sizeof(Geometry::Vertex);
        l_Asset->m_VertexBuffer = std::make_unique<VulkanVertexBuffer>(allocator, uploadContext, l_VertexDataSize, sizeof(Geometry::Vertex), BufferMemoryUsage::GPUOnly,
            meshData.Vertices.data());

        const uint64_t l_IndexDataSize = meshData.Indices.size() * sizeof(uint32_t);
        l_Asset->m_IndexBuffer = std::make_unique<VulkanIndexBuffer>(allocator, uploadContext, l_IndexDataSize, l_Asset->m_IndexCount, IndexType::UInt32, BufferMemoryUsage::GPUOnly,
            meshData.Indices.data());

        return l_Asset;
    }
}