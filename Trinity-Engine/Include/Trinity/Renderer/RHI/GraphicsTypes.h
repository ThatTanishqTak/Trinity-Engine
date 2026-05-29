#pragma once

#include <cstdint>

namespace Trinity
{
    enum class GraphicsBackend
    {
        None = 0,
        Vulkan,
        Metal,
        DirectX12
    };

    enum class Format
    {
        Unknown = 0,

        R8_UNORM,
        RG8_UNORM,
        RGBA8_UNORM,
        RGBA8_SRGB,
        BGRA8_UNORM,
        BGRA8_SRGB,

        R16_SFLOAT,
        RG16_SFLOAT,
        RGBA16_SFLOAT,

        R32_SFLOAT,
        RG32_SFLOAT,
        RGB32_SFLOAT,
        RGBA32_SFLOAT,

        R32_UINT,
        R16_UINT,

        D32_SFLOAT,
        D24_UNORM_S8_UINT,
        D32_SFLOAT_S8_UINT
    };

    enum class ResourceState
    {
        Undefined = 0,
        General,
        VertexBuffer,
        IndexBuffer,
        UniformBuffer,
        ShaderResource,
        RenderTarget,
        DepthStencil,
        CopySource,
        CopyDestination,
        Present
    };

    enum class ShaderStage : uint32_t
    {
        None = 0,
        Vertex = 1 << 0,
        Fragment = 1 << 1,
        Compute = 1 << 2,
        Geometry = 1 << 3,
        TessControl = 1 << 4,
        TessEvaluation = 1 << 5
    };

    inline ShaderStage operator|(ShaderStage left, ShaderStage right) { return static_cast<ShaderStage>(static_cast<uint32_t>(left) | static_cast<uint32_t>(right)); }

    inline bool HasStage(ShaderStage flags, ShaderStage stage) { return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(stage)) != 0; }

    enum class PrimitiveTopology
    {
        TriangleList = 0,
        TriangleStrip,
        LineList,
        LineStrip,
        PointList
    };

    enum class CullMode
    {
        None = 0,
        Front,
        Back
    };

    enum class FrontFace
    {
        CounterClockwise = 0,
        Clockwise
    };

    enum class CompareOp
    {
        Never = 0,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always
    };

    enum class BufferUsage : uint32_t
    {
        None = 0,
        Vertex = 1 << 0,
        Index = 1 << 1,
        Uniform = 1 << 2,
        Storage = 1 << 3,
        TransferSource = 1 << 4,
        TransferDestination = 1 << 5
    };

    inline BufferUsage operator|(BufferUsage left, BufferUsage right) { return static_cast<BufferUsage>(static_cast<uint32_t>(left) | static_cast<uint32_t>(right)); }

    inline bool HasUsage(BufferUsage flags, BufferUsage usage) { return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(usage)) != 0; }

    enum class MemoryUsage
    {
        GpuOnly = 0,
        CpuToGpu,
        GpuToCpu
    };

    enum class TextureUsage : uint32_t
    {
        None = 0,
        Sampled = 1 << 0,
        Storage = 1 << 1,
        RenderTarget = 1 << 2,
        DepthStencil = 1 << 3,
        TransferSource = 1 << 4,
        TransferDestination = 1 << 5
    };

    inline TextureUsage operator|(TextureUsage left, TextureUsage right) { return static_cast<TextureUsage>(static_cast<uint32_t>(left) | static_cast<uint32_t>(right)); }

    inline bool HasUsage(TextureUsage flags, TextureUsage usage) { return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(usage)) != 0; }
}