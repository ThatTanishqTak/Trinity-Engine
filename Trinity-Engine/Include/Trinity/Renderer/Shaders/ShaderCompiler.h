#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Trinity
{
    enum class ShaderTargetFormat
    {
        SPIRV,
        Metal,
        Dxil
    };

    enum class ShaderResourceKind
    {
        Unknown,
        PushConstant,
        UniformBuffer,
        SampledTexture,
        StorageResource,
        Sampler
    };

    struct ShaderBinding
    {
        std::string Name;
        ShaderResourceKind Kind = ShaderResourceKind::Unknown;
        uint32_t Set = 0;
        uint32_t Binding = 0;
        uint32_t Size = 0;
    };

    struct ShaderVertexInput
    {
        std::string Name;
        uint32_t Location = 0;
    };

    struct ShaderReflection
    {
        std::vector<ShaderBinding> Bindings;
        std::vector<ShaderVertexInput> VertexInputs;
        uint32_t PushConstantSize = 0;
    };

    enum class ShaderDiagnosticSeverity
    {
        Info,
        Warning,
        Error
    };

    struct ShaderDiagnostic
    {
        ShaderDiagnosticSeverity Severity = ShaderDiagnosticSeverity::Info;
        std::string File;
        uint32_t Line = 0;
        uint32_t Column = 0;
        std::string Code;
        std::string Message;
    };

    struct ShaderCompileResult
    {
        bool Success = false;
        std::vector<uint8_t> SPIRV;
        std::string Diagnostics;
        std::vector<ShaderDiagnostic> Messages;
        ShaderReflection Reflection;
    };

    class ShaderCompiler
    {
    public:
        ShaderCompiler();
        ~ShaderCompiler();

        ShaderCompiler(const ShaderCompiler&) = delete;
        ShaderCompiler& operator=(const ShaderCompiler&) = delete;

        bool Initialize();
        void Shutdown();

        void SetCacheDirectory(const std::filesystem::path& directory);

        ShaderCompileResult Compile(const std::filesystem::path& searchDirectory, const std::string& moduleName, const std::string& entryPoint, ShaderTargetFormat target = ShaderTargetFormat::SPIRV);

    private:
        struct Implementation;
        std::unique_ptr<Implementation> m_Implementation;
        std::filesystem::path m_CacheDirectory;
    };
}