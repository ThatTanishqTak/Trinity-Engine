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

    class Shader
    {
    public:
        virtual ~Shader() = default;

        const ShaderSpecification& GetSpecification() const { return m_Specification; }

    protected:
        ShaderSpecification m_Specification;
    };
}