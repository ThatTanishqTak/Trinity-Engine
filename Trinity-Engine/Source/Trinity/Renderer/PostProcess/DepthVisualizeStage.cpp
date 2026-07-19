#include <Trinity/Renderer/PostProcess/DepthVisualizeStage.h>

#include <Trinity/Renderer/RHI/GraphicsDevice.h>
#include <Trinity/Renderer/RHI/CommandList.h>
#include <Trinity/Renderer/RHI/Pipeline.h>
#include <Trinity/Renderer/RHI/Shader.h>
#include <Trinity/Renderer/RHI/Texture.h>
#include <Trinity/Renderer/Shaders/ShaderCompiler.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    struct DepthVisualizePush
    {
        float Near;
        float Far;
    };

    bool DepthVisualizeStage::Initialize(GraphicsDevice& device, ShaderCompiler& compiler, const std::filesystem::path& shaderDirectory, Format outputFormat)
    {
        m_Device = &device;

        ShaderCompileResult l_VertexResult = compiler.Compile(shaderDirectory, "DepthVisualize", "vertexMain");
        if (!l_VertexResult.Success)
        {
            ("DepthVisualizeStage: DepthVisualize vertexMain compile failed");

            return false;
        }

        ShaderCompileResult l_FragmentResult = compiler.Compile(shaderDirectory, "DepthVisualize", "fragmentMain");
        if (!l_FragmentResult.Success)
        {
            ("DepthVisualizeStage: DepthVisualize fragmentMain compile failed");

            return false;
        }

        ShaderDescription l_VertexDescription;
        l_VertexDescription.Stage = ShaderStage::Vertex;
        l_VertexDescription.Bytecode = l_VertexResult.SPIRV;
        l_VertexDescription.EntryPoint = "vertexMain";
        l_VertexDescription.DebugName = "DepthVisualize.vertexMain";
        m_VertexShader = device.CreateShader(l_VertexDescription);

        ShaderDescription l_FragmentDescription;
        l_FragmentDescription.Stage = ShaderStage::Fragment;
        l_FragmentDescription.Bytecode = l_FragmentResult.SPIRV;
        l_FragmentDescription.EntryPoint = "fragmentMain";
        l_FragmentDescription.DebugName = "DepthVisualize.fragmentMain";
        m_FragmentShader = device.CreateShader(l_FragmentDescription);

        if (!m_VertexShader.IsValid() || !m_FragmentShader.IsValid())
        {
            ("DepthVisualizeStage: shader creation failed");
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
        l_PipelineDescription.DepthFormat = Format::Unknown;
        l_PipelineDescription.PushConstantSize = sizeof(DepthVisualizePush);
        l_PipelineDescription.Bindings = { l_Binding };
        l_PipelineDescription.ColorFormats = { outputFormat };
        l_PipelineDescription.DebugName = "DepthVisualize";

        m_Pipeline = device.CreatePipeline(l_PipelineDescription);
        if (!m_Pipeline.IsValid())
        {
            ("DepthVisualizeStage: pipeline creation failed");
            Shutdown();

            return false;
        }

        SamplerDescription l_SamplerDescription;
        l_SamplerDescription.LinearFilter = false;
        l_SamplerDescription.LinearMipmap = false;
        l_SamplerDescription.RepeatU = false;
        l_SamplerDescription.RepeatV = false;
        l_SamplerDescription.RepeatW = false;
        l_SamplerDescription.DebugName = "DepthVisualizeSampler";
        m_Sampler = device.CreateSampler(l_SamplerDescription);

        if (!m_Sampler.IsValid())
        {
            ("DepthVisualizeStage: sampler creation failed");
            Shutdown();

            return false;
        }

        ("DepthVisualizeStage: initialized");

        return true;
    }

    void DepthVisualizeStage::Shutdown()
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

    void DepthVisualizeStage::Execute(CommandList& commandList, TextureHandle depthSource, TextureHandle output, uint32_t width, uint32_t height, float nearPlane, float farPlane)
    {
        RenderingAttachment l_Color;
        l_Color.Target = output;
        l_Color.Clear = false;

        RenderingInfo l_Info;
        l_Info.ColorAttachments = &l_Color;
        l_Info.ColorAttachmentCount = 1;
        l_Info.Depth = nullptr;
        l_Info.Width = width;
        l_Info.Height = height;

        commandList.BeginRendering(l_Info);

        Viewport l_Viewport;
        l_Viewport.X = 0.0f;
        l_Viewport.Y = 0.0f;
        l_Viewport.Width = static_cast<float>(width);
        l_Viewport.Height = static_cast<float>(height);
        l_Viewport.MinDepth = 0.0f;
        l_Viewport.MaxDepth = 1.0f;
        commandList.SetViewport(l_Viewport);

        Scissor l_Scissor;
        l_Scissor.X = 0;
        l_Scissor.Y = 0;
        l_Scissor.Width = width;
        l_Scissor.Height = height;
        commandList.SetScissor(l_Scissor);

        DepthVisualizePush l_Push{ nearPlane, farPlane };

        commandList.BindPipeline(m_Pipeline);
        commandList.BindTexture(0, 0, depthSource, m_Sampler);
        commandList.PushConstants(ShaderStage::Vertex | ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(DepthVisualizePush)), &l_Push);
        commandList.Draw(3, 1, 0, 0);

        commandList.EndRendering();
    }
}