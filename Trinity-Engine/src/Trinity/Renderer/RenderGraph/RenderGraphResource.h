#pragma once

#include "Trinity/Renderer/Resources/Buffer.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <limits>
#include <string>

namespace Trinity
{
    enum class RenderGraphResourceType : uint8_t
    {
        None = 0,
        Texture,
        Buffer
    };

    enum class RenderGraphPassType : uint8_t
    {
        Graphics = 0,
        Compute,
        Transfer,
        Present
    };

    enum class RenderGraphAccess : uint16_t
    {
        None = 0,

        ColorAttachmentRead,
        ColorAttachmentWrite,

        DepthStencilRead,
        DepthStencilWrite,

        ShaderSampledRead,
        ShaderStorageRead,
        ShaderStorageWrite,
        ShaderStorageReadWrite,

        TransferRead,
        TransferWrite,

        Present,

        VertexBufferRead,
        IndexBufferRead,
        UniformBufferRead,

        StorageBufferRead,
        StorageBufferWrite,
        StorageBufferReadWrite,

        IndirectCommandRead
    };

    struct RenderGraphResourceHandle
    {
        uint32_t Index = std::numeric_limits<uint32_t>::max();
        RenderGraphResourceType Type = RenderGraphResourceType::None;

        bool IsValid() const { return Type != RenderGraphResourceType::None && Index != std::numeric_limits<uint32_t>::max(); }
        bool IsTexture() const { return IsValid() && Type == RenderGraphResourceType::Texture; }
        bool IsBuffer() const { return IsValid() && Type == RenderGraphResourceType::Buffer; }

        static RenderGraphResourceHandle Invalid() { return {}; }

        static RenderGraphResourceHandle Texture(uint32_t index)
        {
            RenderGraphResourceHandle l_Handle{};
            l_Handle.Index = index;
            l_Handle.Type = RenderGraphResourceType::Texture;

            return l_Handle;
        }

        static RenderGraphResourceHandle Buffer(uint32_t index)
        {
            RenderGraphResourceHandle l_Handle{};
            l_Handle.Index = index;
            l_Handle.Type = RenderGraphResourceType::Buffer;

            return l_Handle;
        }

        bool operator==(const RenderGraphResourceHandle& other) const { return Index == other.Index && Type == other.Type; }
        bool operator!=(const RenderGraphResourceHandle& other) const { return !(*this == other); }
    };

    struct RenderGraphResourceUsage
    {
        RenderGraphResourceHandle Resource;
        RenderGraphAccess Access = RenderGraphAccess::None;
    };

    struct RenderGraphTextureDescription
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t MipLevels = 1;
        uint32_t ArrayLayers = 1;
        uint32_t SampleCount = 1;

        TextureFormat Format = TextureFormat::None;
        TextureUsage Usage = TextureUsage::None;

        bool MatchSwapchainSize = true;
        bool Persistent = false;
        bool Imported = false;
        bool ExternalOutput = false;

        std::string DebugName;
    };

    struct RenderGraphBufferDescription
    {
        uint64_t Size = 0;
        BufferUsage Usage = BufferUsage::None;
        BufferMemoryType MemoryType = BufferMemoryType::GPU;

        bool Persistent = false;
        bool Imported = false;
        bool ExternalOutput = false;

        std::string DebugName;
    };

    struct RenderGraphResourceLifetime
    {
        uint32_t FirstUse = std::numeric_limits<uint32_t>::max();
        uint32_t LastUse = 0;

        uint32_t FirstWriter = std::numeric_limits<uint32_t>::max();
        uint32_t LastWriter = std::numeric_limits<uint32_t>::max();

        bool Imported = false;
        bool Persistent = false;

        bool IsValid() const { return FirstUse != std::numeric_limits<uint32_t>::max() && FirstUse <= LastUse; }
    };
}