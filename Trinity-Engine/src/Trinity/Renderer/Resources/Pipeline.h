#pragma once

#include "Trinity/Renderer/Resources/DescriptorSetLayout.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Trinity
{
    enum class PrimitiveTopology : uint8_t
    {
        TriangleList = 0,
        TriangleStrip,
        LineList,
        LineStrip,
        PointList
    };

    enum class CullMode : uint8_t
    {
        None = 0,
        Front,
        Back,
        FrontAndBack
    };

    enum class DepthCompareOp : uint8_t
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

    enum class VertexAttributeFormat : uint8_t
    {
        Float = 0,
        Float2,
        Float3,
        Float4,
        Int,
        Int2,
        Int3,
        Int4,
        UInt,
        UInt2,
        UInt3,
        UInt4
    };

    enum class BlendFactor : uint8_t
    {
        Zero = 0,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate
    };

    enum class BlendOp : uint8_t
    {
        Add = 0,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    enum class ColorWriteMask : uint8_t
    {
        None = 0,
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,
        RGB = Red | Green | Blue,
        RGBA = Red | Green | Blue | Alpha
    };

    inline ColorWriteMask operator|(ColorWriteMask a, ColorWriteMask b) { return static_cast<ColorWriteMask>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    inline bool operator&(ColorWriteMask a, ColorWriteMask b) { return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0; }

    struct BlendAttachmentState
    {
        bool BlendEnable = false;
        BlendFactor SrcColorFactor = BlendFactor::One;
        BlendFactor DstColorFactor = BlendFactor::Zero;
        BlendOp ColorOp = BlendOp::Add;
        BlendFactor SrcAlphaFactor = BlendFactor::One;
        BlendFactor DstAlphaFactor = BlendFactor::Zero;
        BlendOp AlphaOp = BlendOp::Add;
        ColorWriteMask WriteMask = ColorWriteMask::RGBA;
    };

    struct VertexAttribute
    {
        uint32_t Location = 0;
        uint32_t Binding = 0;
        VertexAttributeFormat Format = VertexAttributeFormat::Float3;
        uint32_t Offset = 0;
    };

    struct PushConstantRange
    {
        ShaderStage Stage = ShaderStage::Vertex;
        uint32_t Offset = 0;
        uint32_t Size = 0;
    };

    struct PipelineSpecification
    {
        std::shared_ptr<Shader> PipelineShader;
        std::vector<VertexAttribute> VertexAttributes;
        uint32_t VertexStride = 0;
        std::vector<PushConstantRange> PushConstants;

        PrimitiveTopology Topology = PrimitiveTopology::TriangleList;
        CullMode CullingMode = CullMode::Back;
        bool DepthTest = true;
        bool DepthWrite = true;
        DepthCompareOp DepthOp = DepthCompareOp::Less;
        bool WireframeMode = false;
        bool DepthBias = false;
        float DepthBiasConstantFactor = 0.0f;
        float DepthBiasSlopeFactor = 0.0f;

        uint32_t SampleCount = 1;
        bool SampleShadingEnable = false;
        float MinSampleShading = 1.0f;

        std::vector<std::shared_ptr<DescriptorSetLayout>> DescriptorSetLayouts;

        std::vector<TextureFormat> ColorAttachmentFormats;
        std::vector<BlendAttachmentState> BlendStates;
        TextureFormat DepthAttachmentFormat = TextureFormat::None;

        std::string DebugName;
    };

    class Pipeline
    {
    public:
        virtual ~Pipeline() = default;

        const PipelineSpecification& GetSpecification() const { return m_Specification; }

    protected:
        PipelineSpecification m_Specification;
    };
}