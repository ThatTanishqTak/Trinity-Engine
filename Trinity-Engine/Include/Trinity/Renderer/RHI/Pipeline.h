#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Renderer/RHI/Handle.h>

namespace Trinity
{
    struct VertexAttribute
    {
        uint32_t Location = 0;
        uint32_t Offset = 0;
        
        Format Format = Format::RGB32_SFLOAT;
    };

    struct VertexLayout
    {
        uint32_t Stride = 0;

        std::vector<VertexAttribute> Attributes;
    };

    struct RasterizerState
    {
        CullMode Cull = CullMode::Back;
        FrontFace Front = FrontFace::CounterClockwise;
        
        bool Wireframe = false;
    };

    struct DepthStencilState
    {
        bool DepthTest = true;
        bool DepthWrite = true;

        CompareOp DepthCompare = CompareOp::Less;
    };

    struct BlendState
    {
        bool Enabled = false;
    };

    enum class ResourceBindingType
    {
        CombinedImageSampler,
        UniformBuffer,
        StorageBuffer,
        StorageImage
    };

    struct ResourceBinding
    {
        uint32_t Set = 0;
        uint32_t Binding = 0;
        ResourceBindingType Type = ResourceBindingType::CombinedImageSampler;
        ShaderStage Stages = ShaderStage::Fragment;
    };

    struct PipelineDescription
    {
        ShaderHandle VertexShader;
        ShaderHandle FragmentShader;
        VertexLayout Vertex;
        PrimitiveTopology Topology = PrimitiveTopology::TriangleList;
        RasterizerState Rasterizer;
        DepthStencilState DepthStencil;
        BlendState Blend;
        Format DepthFormat = Format::Unknown;

        uint32_t PushConstantSize = 0;

        std::vector<ResourceBinding> Bindings;
        std::vector<Format> ColorFormats;

        std::string DebugName;
    };
}