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
        None = 0,
        Vertex = 1 << 0,
        Fragment = 1 << 1,
        Compute = 1 << 2
    };

    inline ShaderStageFlags operator|(ShaderStageFlags a, ShaderStageFlags b) { return static_cast<ShaderStageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
    inline bool operator&(ShaderStageFlags a, ShaderStageFlags b) { return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0; }

    enum class AttachmentLoadOp : uint8_t
    {
        Load,
        Clear,
        DontCare
    };

    enum class AttachmentStoreOp : uint8_t
    {
        Store,
        DontCare
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

    struct ClearColorValue
    {
        float R = 0.0f;
        float G = 0.0f;
        float B = 0.0f;
        float A = 1.0f;
    };

    struct ColorAttachmentInfo
    {
        Texture* Image = nullptr;
        AttachmentLoadOp LoadOp = AttachmentLoadOp::Clear;
        AttachmentStoreOp StoreOp = AttachmentStoreOp::Store;
        ClearColorValue ClearColor;
    };

    struct DepthAttachmentInfo
    {
        Texture* Image = nullptr;
        AttachmentLoadOp LoadOp = AttachmentLoadOp::Clear;
        AttachmentStoreOp StoreOp = AttachmentStoreOp::Store;
        float ClearDepth = 1.0f;
    };

    struct RenderingInfo
    {
        int32_t RenderAreaX = 0;
        int32_t RenderAreaY = 0;
        uint32_t RenderAreaWidth = 0;
        uint32_t RenderAreaHeight = 0;

        const ColorAttachmentInfo* ColorAttachments = nullptr;
        uint32_t ColorAttachmentCount = 0;

        const DepthAttachmentInfo* DepthAttachment = nullptr;
    };

    class CommandBuffer
    {
    public:
        virtual ~CommandBuffer() = default;

        virtual void BeginRendering(const RenderingInfo& info) = 0;
        virtual void EndRendering() = 0;

        virtual void BindPipeline(Pipeline& pipeline) = 0;
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