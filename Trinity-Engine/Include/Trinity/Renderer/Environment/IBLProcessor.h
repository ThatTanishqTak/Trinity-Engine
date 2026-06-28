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

    // Precomputes diffuse irradiance, specular prefilter, and the BRDF LUT from an equirectangular HDR environment map Convolution passes render into cube faces / mips via subresource render targets
    class IBLProcessor
    {
    public:
        IBLProcessor() = default;
        ~IBLProcessor() = default;

        IBLProcessor(const IBLProcessor&) = delete;
        IBLProcessor& operator=(const IBLProcessor&) = delete;

        bool Initialize(GraphicsDevice& device, ShaderCompiler& compiler, const std::filesystem::path& shaderDirectory);
        void Shutdown();

        void GenerateIrradiance(CommandList& commandList, TextureHandle equirect, TextureHandle irradiance, uint32_t size);
        void GeneratePrefilter(CommandList& commandList, TextureHandle equirect, TextureHandle prefiltered, uint32_t baseSize, uint32_t mipCount);
        void GenerateBrdfLut(CommandList& commandList, TextureHandle brdfLut, uint32_t size);

        // Clears a cube target to black and leaves it in ShaderResource (used when no environment map is present).
        void ClearCube(CommandList& commandList, TextureHandle target, uint32_t baseSize, uint32_t mipCount);

    private:
        GraphicsDevice* m_Device = nullptr;

        ShaderHandle m_IrradianceVertex;
        ShaderHandle m_IrradianceFragment;
        PipelineHandle m_IrradiancePipeline;

        ShaderHandle m_PrefilterVertex;
        ShaderHandle m_PrefilterFragment;
        PipelineHandle m_PrefilterPipeline;

        ShaderHandle m_BrdfVertex;
        ShaderHandle m_BrdfFragment;
        PipelineHandle m_BrdfPipeline;

        SamplerHandle m_EquirectSampler;
    };
}