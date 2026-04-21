#pragma once

#include "Trinity/Renderer/Resources/Buffer.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace Trinity
{
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TextureCoordinate;
    };

    class Mesh
    {
    public:
        static std::shared_ptr<Mesh> Create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

        std::shared_ptr<Buffer> GetVertexBuffer() const { return m_VertexBuffer; }
        std::shared_ptr<Buffer> GetIndexBuffer() const { return m_IndexBuffer; }
        uint32_t GetIndexCount() const { return m_IndexCount; }
        uint32_t GetVertexCount() const { return m_VertexCount; }

    public:
        std::shared_ptr<Texture> AlbedoTexture;

    private:
        std::shared_ptr<Buffer> m_VertexBuffer;
        std::shared_ptr<Buffer> m_IndexBuffer;

        uint32_t m_IndexCount = 0;
        uint32_t m_VertexCount = 0;
    };

    namespace Primitives
    {
        std::shared_ptr<Mesh> CreateCube();
    }
}