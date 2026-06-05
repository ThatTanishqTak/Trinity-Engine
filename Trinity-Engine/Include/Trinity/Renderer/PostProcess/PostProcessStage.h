#pragma once

#include <cstdint>
#include <filesystem>

#include <Trinity/Renderer/RHI/Handle.h>
#include <Trinity/Renderer/RHI/GraphicsTypes.h>

namespace Trinity
{
    class GraphicsDevice;
    class ShaderCompiler;
    class CommandList;

    class PostProcessStage
    {
    public:
        PostProcessStage() = default;
        ~PostProcessStage() = default;

        PostProcessStage(const PostProcessStage&) = delete;
        PostProcessStage& operator=(const PostProcessStage&) = delete;

        bool Initialize(GraphicsDevice& device, ShaderCompiler& compiler, const std::filesystem::path& shaderDirectory, Format outputFormat);
        void Shutdown();

        void Execute(CommandList& commandList, TextureHandle hdrSource, TextureHandle output, uint32_t width, uint32_t height, float exposure);

    private:
        GraphicsDevice* m_Device = nullptr;
        ShaderHandle m_VertexShader;
        ShaderHandle m_FragmentShader;
        PipelineHandle m_Pipeline;
        SamplerHandle m_Sampler;
    };
}