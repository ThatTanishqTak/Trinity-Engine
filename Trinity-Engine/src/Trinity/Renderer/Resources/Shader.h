#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Trinity
{
    enum class ShaderStage : uint8_t
    {
        None = 0,
        Vertex,
        Fragment,
        Compute,
        Geometry,
        TessControl,
        TessEval,
        Task,
        Mesh
    };

    enum class ShaderStageFlags : uint32_t
    {
        None = 0,
        Vertex = 1 << 0,
        Fragment = 1 << 1,
        Compute = 1 << 2,
        Geometry = 1 << 3,
        TessControl = 1 << 4,
        TessEval = 1 << 5,
        Task = 1 << 6,
        Mesh = 1 << 7,
        AllGraphics = Vertex | Fragment | Geometry | TessControl | TessEval,
        All = 0xFFFFFFFF
    };

    inline ShaderStageFlags operator|(ShaderStageFlags a, ShaderStageFlags b) { return static_cast<ShaderStageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
    inline ShaderStageFlags& operator|=(ShaderStageFlags& a, ShaderStageFlags b) { a = a | b; return a; }
    inline bool operator&(ShaderStageFlags a, ShaderStageFlags b) { return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0; }

    inline ShaderStageFlags ToShaderStageFlags(ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::Vertex:
                return ShaderStageFlags::Vertex;
            case ShaderStage::Fragment:
                return ShaderStageFlags::Fragment;
            case ShaderStage::Compute:
                return ShaderStageFlags::Compute;
            case ShaderStage::Geometry:
                return ShaderStageFlags::Geometry;
            case ShaderStage::TessControl:
                return ShaderStageFlags::TessControl;
            case ShaderStage::TessEval:
                return ShaderStageFlags::TessEval;
            case ShaderStage::Task:
                return ShaderStageFlags::Task;
            case ShaderStage::Mesh:
                return ShaderStageFlags::Mesh;
            default:
                return ShaderStageFlags::None;
        }
    }

    struct ShaderModuleSpecification
    {
        ShaderStage Stage = ShaderStage::None;
        std::string SpvPath;
        std::vector<uint32_t> SpvBlob;
        std::string EntryPoint = "main";
    };

    struct ShaderSpecification
    {
        std::vector<ShaderModuleSpecification> Modules;
        uint64_t PermutationHash = 0;
        std::string DebugName;
    };

    enum class ReflectedResourceType : uint8_t
    {
        UniformBuffer = 0,
        StorageBuffer,
        SampledImage,
        StorageImage,
        Sampler,
        CombinedImageSampler,
        InputAttachment,
        AccelerationStructure
    };

    enum class ReflectedVertexFormat : uint8_t
    {
        Unknown = 0,
        Float,
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

    struct ReflectedDescriptorBinding
    {
        uint32_t Set = 0;
        uint32_t Binding = 0;
        ReflectedResourceType Type = ReflectedResourceType::UniformBuffer;
        uint32_t Count = 1;
        bool IsRuntimeArray = false;
        ShaderStageFlags Stages = ShaderStageFlags::None;
        std::string Name;
    };

    struct ReflectedPushConstantRange
    {
        uint32_t Offset = 0;
        uint32_t Size = 0;
        ShaderStageFlags Stages = ShaderStageFlags::None;
        std::string Name;
    };

    struct ReflectedVertexInput
    {
        uint32_t Location = 0;
        ReflectedVertexFormat Format = ReflectedVertexFormat::Unknown;
        std::string Name;
    };

    struct ReflectedSpecializationConstant
    {
        uint32_t ConstantID = 0;
        uint32_t Size = 0;
        ShaderStageFlags Stages = ShaderStageFlags::None;
        std::string Name;
    };

    struct ReflectedEntryPoint
    {
        ShaderStage Stage = ShaderStage::None;
        std::string Name;
    };

    struct ShaderReflection
    {
        std::vector<ReflectedDescriptorBinding> DescriptorBindings;
        std::vector<ReflectedPushConstantRange> PushConstants;
        std::vector<ReflectedVertexInput> VertexInputs;
        std::vector<ReflectedSpecializationConstant> SpecializationConstants;
        std::vector<ReflectedEntryPoint> EntryPoints;
    };

    class Shader
    {
    public:
        virtual ~Shader() = default;

        virtual const ShaderReflection& GetReflection() const = 0;

        const ShaderSpecification& GetSpecification() const { return m_Specification; }

    protected:
        ShaderSpecification m_Specification;
    };
}