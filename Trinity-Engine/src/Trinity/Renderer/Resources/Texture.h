#pragma once

#include <cstdint>
#include <string>

namespace Trinity
{
    enum class TextureFormat : uint16_t
    {
        None = 0,

        R8,
        RG8,
        RGBA8,
        RGBA8_SRGB,
        BGRA8,
        BGRA8_SRGB,

        R16F,
        RG16F,
        RGBA16F,

        R32F,
        RG32F,
        RGBA32F,

        R11G11B10F,
        RGB10A2,

        Depth16Unorm,
        Depth32F,
        Depth24Stencil8,
        Depth32FStencil8,

        BC1,
        BC2,
        BC3,
        BC4,
        BC5,
        BC6H,
        BC7,

        ASTC_4x4
    };

    enum class TextureUsage : uint32_t
    {
        None = 0,
        Sampled = 1 << 0,
        Storage = 1 << 1,
        ColorAttachment = 1 << 2,
        DepthStencilAttachment = 1 << 3,
        TransferSource = 1 << 4,
        TransferDestination = 1 << 5,
        InputAttachment = 1 << 6
    };

    inline TextureUsage operator|(TextureUsage a, TextureUsage b) { return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
    inline TextureUsage& operator|=(TextureUsage& a, TextureUsage b)
    {
        a = a | b;
        return a;
    }
    inline bool operator&(TextureUsage a, TextureUsage b) { return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0; }

    struct TextureSpecification
    {
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint32_t Depth = 1;
        uint32_t MipLevels = 1;
        uint32_t ArrayLayers = 1;
        uint32_t Samples = 1;

        TextureFormat Format = TextureFormat::RGBA8;
        TextureUsage Usage = TextureUsage::Sampled;

        bool IsCubemap = false;
        bool IsVolume = false;

        std::string DebugName;
    };

    class Texture
    {
    public:
        virtual ~Texture() = default;

        virtual uint64_t GetOpaqueHandle() const = 0;
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetDepth() const { return 1; }
        virtual uint32_t GetMipLevels() const = 0;
        virtual uint32_t GetArrayLayers() const { return 1; }
        virtual uint32_t GetSampleCount() const { return 1; }
        virtual bool IsCubemap() const { return false; }
        virtual bool IsVolume() const { return false; }

        const TextureSpecification& GetSpecification() const { return m_Specification; }

    protected:
        TextureSpecification m_Specification;
    };
}