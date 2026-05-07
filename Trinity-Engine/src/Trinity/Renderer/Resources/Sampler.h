#pragma once

#include <cstdint>
#include <string>

namespace Trinity
{
    enum class SamplerFilter : uint8_t
    {
        Nearest = 0,
        Linear
    };

    enum class SamplerAddressMode : uint8_t
    {
        Repeat = 0,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder
    };

    enum class SamplerMipmapMode : uint8_t
    {
        Nearest = 0,
        Linear
    };

    enum class BorderColor : uint8_t
    {
        FloatTransparentBlack = 0,
        FloatOpaqueBlack,
        FloatOpaqueWhite,
        IntTransparentBlack,
        IntOpaqueBlack,
        IntOpaqueWhite
    };

    enum class CompareOp : uint8_t
    {
        Never = 0,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    struct SamplerSpecification
    {
        SamplerFilter MinFilter = SamplerFilter::Linear;
        SamplerFilter MagFilter = SamplerFilter::Linear;

        SamplerMipmapMode MipmapMode = SamplerMipmapMode::Linear;

        SamplerAddressMode AddressModeU = SamplerAddressMode::Repeat;
        SamplerAddressMode AddressModeV = SamplerAddressMode::Repeat;
        SamplerAddressMode AddressModeW = SamplerAddressMode::Repeat;

        float MipLodBias = 0.0f;
        float MinLod = 0.0f;
        float MaxLod = 1000.0f;

        bool AnisotropyEnable = false;
        float MaxAnisotropy = 1.0f;

        bool CompareEnable = false;
        CompareOp Compare = CompareOp::Always;

        BorderColor Border = BorderColor::FloatOpaqueBlack;

        bool UnnormalizedCoordinates = false;

        std::string DebugName;
    };

    class Sampler
    {
    public:
        virtual ~Sampler() = default;

        const SamplerSpecification& GetSpecification() const { return m_Specification; }

    protected:
        SamplerSpecification m_Specification;
    };
}