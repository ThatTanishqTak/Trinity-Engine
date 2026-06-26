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

    // Fullscreen pass that linearizes the depth buffer into a grayscale color target for the editor's render-target viewer
    class DepthVisualizeStage
    {
    public:
        DepthVisualizeStage() = default;
        ~DepthVisualizeStage() = default;

        DepthVisualizeStage(const DepthVisualizeStage&) = delete;
        DepthVisualizeStage& operator=(const DepthVisualizeStage&) = delete;

        bool Initialize(GraphicsDevice& device, ShaderCompiler& compiler, const std::filesystem::path& shaderDirectory, Format outputFormat);
        void Shutdown();

        void Execute(CommandList& commandList, TextureHandle depthSource, TextureHandle output, uint32_t width, uint32_t height, float nearPlane, float farPlane);

    private:
        GraphicsDevice* m_Device = nullptr;
        ShaderHandle m_VertexShader;
        ShaderHandle m_FragmentShader;
        PipelineHandle m_Pipeline;
        SamplerHandle m_Sampler;
    };
}