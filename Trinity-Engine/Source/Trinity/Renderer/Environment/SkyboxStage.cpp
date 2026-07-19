#include <Trinity/Renderer/Environment/SkyboxStage.h>

#include <Trinity/Renderer/RHI/GraphicsDevice.h>
#include <Trinity/Renderer/RHI/CommandList.h>
#include <Trinity/Renderer/RHI/Pipeline.h>
#include <Trinity/Renderer/RHI/Shader.h>
#include <Trinity/Renderer/RHI/Texture.h>
#include <Trinity/Renderer/Shaders/ShaderCompiler.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    struct SkyboxPush
    {
        glm::mat4 InverseViewProjection;
        glm::vec4 CameraPositionIntensity;
    };

    bool SkyboxStage::Initialize(GraphicsDevice& device, ShaderCompiler& compiler, const std::filesystem::path& shaderDirectory, Format colorFormat, Format depthFormat)
    {
        m_Device = &device;

        ShaderCompileResult l_VertexResult = compiler.Compile(shaderDirectory, "Skybox", "vertexMain");
        ShaderCompileResult l_FragmentResult = compiler.Compile(shaderDirectory, "Skybox", "fragmentMain");
        if (!l_VertexResult.Success || !l_FragmentResult.Success)
        {
            ("SkyboxStage: shader compilation failed");

            return false;
        }

        ShaderDescription l_VertexDescription;
        l_VertexDescription.Stage = ShaderStage::Vertex;
        l_VertexDescription.Bytecode = l_VertexResult.SPIRV;
        l_VertexDescription.EntryPoint = "vertexMain";
        l_VertexDescription.DebugName = "Skybox.vertexMain";
        m_VertexShader = device.CreateShader(l_VertexDescription);

        ShaderDescription l_FragmentDescription;
        l_FragmentDescription.Stage = ShaderStage::Fragment;
        l_FragmentDescription.Bytecode = l_FragmentResult.SPIRV;
        l_FragmentDescription.EntryPoint = "fragmentMain";
        l_FragmentDescription.DebugName = "Skybox.fragmentMain";
        m_FragmentShader = device.CreateShader(l_FragmentDescription);

        if (!m_VertexShader.IsValid() || !m_FragmentShader.IsValid())
        {
            ("SkyboxStage: shader creation failed");
            Shutdown();

            return false;
        }

        ResourceBinding l_Binding;
        l_Binding.Set = 0;
        l_Binding.Binding = 0;
        l_Binding.Type = ResourceBindingType::CombinedImageSampler;
        l_Binding.Stages = ShaderStage::Fragment;

        PipelineDescription l_PipelineDescription;
        l_PipelineDescription.VertexShader = m_VertexShader;
        l_PipelineDescription.FragmentShader = m_FragmentShader;
        l_PipelineDescription.Topology = PrimitiveTopology::TriangleList;
        l_PipelineDescription.Rasterizer.Cull = CullMode::None;
        l_PipelineDescription.DepthStencil.DepthTest = false;
        l_PipelineDescription.DepthStencil.DepthWrite = false;
        l_PipelineDescription.DepthFormat = depthFormat;
        l_PipelineDescription.PushConstantSize = sizeof(SkyboxPush);
        l_PipelineDescription.Bindings = { l_Binding };
        l_PipelineDescription.ColorFormats = { colorFormat };
        l_PipelineDescription.DebugName = "Skybox";

        m_Pipeline = device.CreatePipeline(l_PipelineDescription);
        if (!m_Pipeline.IsValid())
        {
            ("SkyboxStage: pipeline creation failed");
            Shutdown();

            return false;
        }

        SamplerDescription l_SamplerDescription;
        l_SamplerDescription.LinearFilter = true;
        l_SamplerDescription.LinearMipmap = false;
        l_SamplerDescription.RepeatU = true;
        l_SamplerDescription.RepeatV = false;
        l_SamplerDescription.RepeatW = false;
        l_SamplerDescription.DebugName = "SkyboxSampler";
        m_Sampler = device.CreateSampler(l_SamplerDescription);

        if (!m_Sampler.IsValid())
        {
            ("SkyboxStage: sampler creation failed");
            Shutdown();

            return false;
        }

        ("SkyboxStage: initialized");

        return true;
    }

    void SkyboxStage::Shutdown()
    {
        if (m_Device == nullptr)
        {
            return;
        }

        if (m_Sampler.IsValid())
        {
            m_Device->DestroySampler(m_Sampler);
            m_Sampler = SamplerHandle{};
        }

        if (m_Pipeline.IsValid())
        {
            m_Device->DestroyPipeline(m_Pipeline);
            m_Pipeline = PipelineHandle{};
        }

        if (m_FragmentShader.IsValid())
        {
            m_Device->DestroyShader(m_FragmentShader);
            m_FragmentShader = ShaderHandle{};
        }

        if (m_VertexShader.IsValid())
        {
            m_Device->DestroyShader(m_VertexShader);
            m_VertexShader = ShaderHandle{};
        }
    }

    void SkyboxStage::Record(CommandList& commandList, TextureHandle environment, const glm::mat4& inverseViewProjection, const glm::vec3& cameraPosition, float intensity)
    {
        if (!m_Pipeline.IsValid() || !environment.IsValid())
        {
            return;
        }

        SkyboxPush l_Push;
        l_Push.InverseViewProjection = inverseViewProjection;
        l_Push.CameraPositionIntensity = glm::vec4(cameraPosition, intensity);

        commandList.BindPipeline(m_Pipeline);
        commandList.BindTexture(0, 0, environment, m_Sampler);
        commandList.PushConstants(ShaderStage::Vertex | ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(SkyboxPush)), &l_Push);
        commandList.Draw(3, 1, 0, 0);
    }
}