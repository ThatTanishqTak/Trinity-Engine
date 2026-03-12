#pragma once

#include "Trinity/Geometry/Geometry.h"

#include <memory>
#include <string>
#include <cstdint>

namespace Trinity
{
    class VertexBuffer;
    class IndexBuffer;
    class VulkanAllocator;
    class VulkanUploadContext;

    class MeshAsset
    {
    public:
        MeshAsset() = default;

        MeshAsset(const MeshAsset&) = delete;
        MeshAsset& operator=(const MeshAsset&) = delete;

        static std::shared_ptr<MeshAsset> CreateFromMeshData(const Geometry::MeshData& meshData, VulkanAllocator& allocator, VulkanUploadContext& uploadContext,
            const std::string& name = "");

        VertexBuffer& GetVertexBuffer() const { return *m_VertexBuffer; }
        IndexBuffer& GetIndexBuffer() const { return *m_IndexBuffer; }

        uint32_t GetIndexCount() const { return m_IndexCount; }
        uint32_t GetVertexCount() const { return m_VertexCount; }

        const std::string& GetName() const { return m_Name; }

        bool IsValid() const { return m_VertexBuffer != nullptr && m_IndexBuffer != nullptr; }

    private:
        std::unique_ptr<VertexBuffer> m_VertexBuffer;
        std::unique_ptr<IndexBuffer> m_IndexBuffer;

        uint32_t m_VertexCount = 0;
        uint32_t m_IndexCount = 0;

        std::string m_Name;
    };
}