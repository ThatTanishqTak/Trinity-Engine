#pragma once

#include <cstdint>

namespace Trinity
{
    class Buffer;
    class Pipeline;
    class Texture;
    class Sampler;

    enum class IndexType : uint8_t
    {
        UInt16,
        UInt32
    };

    enum class ShaderStageFlags : uint32_t
    {
        Vertex = 1 << 0,
        Fragment = 1 << 1,
        Compute = 1 << 2
    };

    struct Viewport
    {
        float X;
        float Y;
        float Width;
        float Height;
        float MinDepth;
        float MaxDepth;
    };

    struct ScissorRect
    {
        int32_t X;
        int32_t Y;

        uint32_t Width;
        uint32_t Height;
    };

    class CommandBuffer
    {
    public:
        virtual ~CommandBuffer() = default;

        virtual void BindPipeline() = 0;
        virtual void BindVertexBuffer(uint32_t binding, Buffer& buffer, uint64_t offset = 0) = 0;
        virtual void BindIndexBuffer(Buffer& buffer, IndexType type, uint64_t offset = 0) = 0;
        virtual void BindTexture(uint32_t set, uint32_t binding, Texture& texture, Sampler& sampler) = 0;
        virtual void BindUniformBuffer(uint32_t set, uint32_t binding, Buffer& buffer, uint64_t offset, uint64_t range) = 0;

        virtual void PushConstants(ShaderStageFlags stages, uint32_t offset, uint32_t size, const void* data) = 0;

        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const ScissorRect& scissor) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
    };
}