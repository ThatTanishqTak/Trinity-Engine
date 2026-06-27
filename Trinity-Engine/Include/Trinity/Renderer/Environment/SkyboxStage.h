#pragma once

#include <filesystem>

#include <glm/glm.hpp>

#include <Trinity/Renderer/RHI/Handle.h>
#include <Trinity/Renderer/RHI/GraphicsTypes.h>

namespace Trinity
{
    class GraphicsDevice;
    class ShaderCompiler;
    class CommandList;

    // Draws an equirectangular HDR environment as the scene background recorded inside the scene pass
    class SkyboxStage
    {
    public:
        SkyboxStage() = default;
        ~SkyboxStage() = default;

        SkyboxStage(const SkyboxStage&) = delete;
        SkyboxStage& operator=(const SkyboxStage&) = delete;

        bool Initialize(GraphicsDevice& device, ShaderCompiler& compiler, const std::filesystem::path& shaderDirectory, Format colorFormat, Format depthFormat);
        void Shutdown();

        void Record(CommandList& commandList, TextureHandle environment, const glm::mat4& inverseViewProjection, const glm::vec3& cameraPosition, float intensity);

    private:
        GraphicsDevice* m_Device = nullptr;
        ShaderHandle m_VertexShader;
        ShaderHandle m_FragmentShader;
        PipelineHandle m_Pipeline;
        SamplerHandle m_Sampler;
    };
}