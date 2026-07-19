#include <Trinity/Renderer/Environment/IBLProcessor.h>

#include <glm/glm.hpp>

#include <Trinity/Renderer/RHI/GraphicsDevice.h>
#include <Trinity/Renderer/RHI/CommandList.h>
#include <Trinity/Renderer/RHI/Pipeline.h>
#include <Trinity/Renderer/RHI/Shader.h>
#include <Trinity/Renderer/RHI/Texture.h>
#include <Trinity/Renderer/Shaders/ShaderCompiler.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    struct FacePush
    {
        glm::vec4 Forward;  // w = roughness (prefilter only)
        glm::vec4 Right;
        glm::vec4 Up;
    };

    // Standard cubemap face orientation: direction = Forward + u*Right + v*Up, with u,v in [-1,1].
    struct FaceBasis
    {
        glm::vec3 Forward;
        glm::vec3 Right;
        glm::vec3 Up;
    };

    static const FaceBasis k_Faces[6] =
    {
        { {  1.0f,  0.0f,  0.0f }, {  0.0f,  0.0f, -1.0f }, {  0.0f, -1.0f,  0.0f } }, // +X
        { { -1.0f,  0.0f,  0.0f }, {  0.0f,  0.0f,  1.0f }, {  0.0f, -1.0f,  0.0f } }, // -X
        { {  0.0f,  1.0f,  0.0f }, {  1.0f,  0.0f,  0.0f }, {  0.0f,  0.0f,  1.0f } }, // +Y
        { {  0.0f, -1.0f,  0.0f }, {  1.0f,  0.0f,  0.0f }, {  0.0f,  0.0f, -1.0f } }, // -Y
        { {  0.0f,  0.0f,  1.0f }, {  1.0f,  0.0f,  0.0f }, {  0.0f, -1.0f,  0.0f } }, // +Z
        { {  0.0f,  0.0f, -1.0f }, { -1.0f,  0.0f,  0.0f }, {  0.0f, -1.0f,  0.0f } }  // -Z
    };

    static PipelineHandle BuildFullscreenPipeline(GraphicsDevice& device, ShaderHandle vertex, ShaderHandle fragment, Format colorFormat, uint32_t pushSize, bool hasTexture, const char* debugName)
    {
        PipelineDescription l_Description;
        l_Description.VertexShader = vertex;
        l_Description.FragmentShader = fragment;
        l_Description.Topology = PrimitiveTopology::TriangleList;
        l_Description.Rasterizer.Cull = CullMode::None;
        l_Description.DepthStencil.DepthTest = false;
        l_Description.DepthStencil.DepthWrite = false;
        l_Description.DepthFormat = Format::Unknown;
        l_Description.PushConstantSize = pushSize;
        l_Description.ColorFormats = { colorFormat };
        l_Description.DebugName = debugName;

        if (hasTexture)
        {
            ResourceBinding l_Binding;
            l_Binding.Set = 0;
            l_Binding.Binding = 0;
            l_Binding.Type = ResourceBindingType::CombinedImageSampler;
            l_Binding.Stages = ShaderStage::Fragment;
            l_Description.Bindings = { l_Binding };
        }

        return device.CreatePipeline(l_Description);
    }

    static ShaderHandle BuildShader(GraphicsDevice& device, ShaderCompiler& compiler, const std::filesystem::path& dir, const char* name, const char* entry, ShaderStage stage)
    {
        ShaderCompileResult l_Result = compiler.Compile(dir, name, entry);
        if (!l_Result.Success)
        {


            return ShaderHandle{};
        }

        ShaderDescription l_Description;
        l_Description.Stage = stage;
        l_Description.Bytecode = l_Result.SPIRV;
        l_Description.EntryPoint = entry;
        l_Description.DebugName = std::string(name) + "." + entry;

        return device.CreateShader(l_Description);
    }

    bool IBLProcessor::Initialize(GraphicsDevice& device, ShaderCompiler& compiler, const std::filesystem::path& shaderDirectory)
    {
        m_Device = &device;

        m_IrradianceVertex = BuildShader(device, compiler, shaderDirectory, "IrradianceConvolve", "vertexMain", ShaderStage::Vertex);
        m_IrradianceFragment = BuildShader(device, compiler, shaderDirectory, "IrradianceConvolve", "fragmentMain", ShaderStage::Fragment);
        m_PrefilterVertex = BuildShader(device, compiler, shaderDirectory, "Prefilter", "vertexMain", ShaderStage::Vertex);
        m_PrefilterFragment = BuildShader(device, compiler, shaderDirectory, "Prefilter", "fragmentMain", ShaderStage::Fragment);
        m_BrdfVertex = BuildShader(device, compiler, shaderDirectory, "BrdfLut", "vertexMain", ShaderStage::Vertex);
        m_BrdfFragment = BuildShader(device, compiler, shaderDirectory, "BrdfLut", "fragmentMain", ShaderStage::Fragment);

        if (!m_IrradianceVertex.IsValid() || !m_IrradianceFragment.IsValid() || !m_PrefilterVertex.IsValid() || !m_PrefilterFragment.IsValid() || !m_BrdfVertex.IsValid() || !m_BrdfFragment.IsValid())
        {
            Shutdown();

            return false;
        }

        m_IrradiancePipeline = BuildFullscreenPipeline(device, m_IrradianceVertex, m_IrradianceFragment, Format::RGBA16_SFLOAT, sizeof(FacePush), true, "Irradiance");
        m_PrefilterPipeline = BuildFullscreenPipeline(device, m_PrefilterVertex, m_PrefilterFragment, Format::RGBA16_SFLOAT, sizeof(FacePush), true, "Prefilter");
        m_BrdfPipeline = BuildFullscreenPipeline(device, m_BrdfVertex, m_BrdfFragment, Format::RG16_SFLOAT, 0, false, "BrdfLut");

        if (!m_IrradiancePipeline.IsValid() || !m_PrefilterPipeline.IsValid() || !m_BrdfPipeline.IsValid())
        {
            Shutdown();

            return false;
        }

        SamplerDescription l_SamplerDescription;
        l_SamplerDescription.LinearFilter = true;
        l_SamplerDescription.LinearMipmap = false;
        l_SamplerDescription.RepeatU = true;
        l_SamplerDescription.RepeatV = false;
        l_SamplerDescription.RepeatW = false;
        l_SamplerDescription.DebugName = "IBLEquirectSampler";
        m_EquirectSampler = device.CreateSampler(l_SamplerDescription);

        if (!m_EquirectSampler.IsValid())
        {
            Shutdown();

            return false;
        }



        return true;
    }

    void IBLProcessor::Shutdown()
    {
        if (m_Device == nullptr)
        {
            return;
        }

        if (m_EquirectSampler.IsValid())
        {
            m_Device->DestroySampler(m_EquirectSampler);
            m_EquirectSampler = SamplerHandle{};
        }

        if (m_IrradiancePipeline.IsValid())
        {
            m_Device->DestroyPipeline(m_IrradiancePipeline);
            m_IrradiancePipeline = PipelineHandle{};
        }
        
        if (m_PrefilterPipeline.IsValid())
        {
            m_Device->DestroyPipeline(m_PrefilterPipeline);
            m_PrefilterPipeline = PipelineHandle{};
        }

        if (m_BrdfPipeline.IsValid())
        {
            m_Device->DestroyPipeline(m_BrdfPipeline);
            m_BrdfPipeline = PipelineHandle{};
        }

        ShaderHandle l_Shaders[] = { m_IrradianceVertex, m_IrradianceFragment, m_PrefilterVertex, m_PrefilterFragment, m_BrdfVertex, m_BrdfFragment };
        for (ShaderHandle& it_Shader : l_Shaders)
        {
            if (it_Shader.IsValid())
            {
                m_Device->DestroyShader(it_Shader);
            }
        }

        m_IrradianceVertex = m_IrradianceFragment = ShaderHandle{};
        m_PrefilterVertex = m_PrefilterFragment = ShaderHandle{};
        m_BrdfVertex = m_BrdfFragment = ShaderHandle{};
    }

    static void SetFullViewport(CommandList& commandList, uint32_t size)
    {
        Viewport l_Viewport;
        l_Viewport.X = 0.0f;
        l_Viewport.Y = 0.0f;
        l_Viewport.Width = static_cast<float>(size);
        l_Viewport.Height = static_cast<float>(size);
        l_Viewport.MinDepth = 0.0f;
        l_Viewport.MaxDepth = 1.0f;
        commandList.SetViewport(l_Viewport);

        Scissor l_Scissor;
        l_Scissor.X = 0;
        l_Scissor.Y = 0;
        l_Scissor.Width = size;
        l_Scissor.Height = size;
        commandList.SetScissor(l_Scissor);
    }

    static void BeginFace(CommandList& commandList, TextureHandle target, uint32_t mip, uint32_t face, uint32_t size, bool clear)
    {
        RenderingAttachment l_Color;
        l_Color.Target = target;
        l_Color.Clear = clear;
        l_Color.ClearColor[0] = 0.0f;
        l_Color.ClearColor[1] = 0.0f;
        l_Color.ClearColor[2] = 0.0f;
        l_Color.ClearColor[3] = 1.0f;
        l_Color.MipLevel = mip;
        l_Color.ArrayLayer = face;

        RenderingInfo l_Info;
        l_Info.ColorAttachments = &l_Color;
        l_Info.ColorAttachmentCount = 1;
        l_Info.Depth = nullptr;
        l_Info.Width = size;
        l_Info.Height = size;

        commandList.BeginRendering(l_Info);
        SetFullViewport(commandList, size);
    }

    void IBLProcessor::GenerateIrradiance(CommandList& commandList, TextureHandle equirect, TextureHandle irradiance, uint32_t size)
    {
        commandList.TransitionTexture(irradiance, ResourceState::Undefined, ResourceState::RenderTarget);

        for (uint32_t l_Face = 0; l_Face < 6; ++l_Face)
        {
            BeginFace(commandList, irradiance, 0, l_Face, size, true);

            FacePush l_Push;
            l_Push.Forward = glm::vec4(k_Faces[l_Face].Forward, 0.0f);
            l_Push.Right = glm::vec4(k_Faces[l_Face].Right, 0.0f);
            l_Push.Up = glm::vec4(k_Faces[l_Face].Up, 0.0f);

            commandList.BindPipeline(m_IrradiancePipeline);
            commandList.BindTexture(0, 0, equirect, m_EquirectSampler);
            commandList.PushConstants(ShaderStage::Vertex | ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(FacePush)), &l_Push);
            commandList.Draw(3, 1, 0, 0);

            commandList.EndRendering();
        }

        commandList.TransitionTexture(irradiance, ResourceState::RenderTarget, ResourceState::ShaderResource);
    }

    void IBLProcessor::GeneratePrefilter(CommandList& commandList, TextureHandle equirect, TextureHandle prefiltered, uint32_t baseSize, uint32_t mipCount)
    {
        commandList.TransitionTexture(prefiltered, ResourceState::Undefined, ResourceState::RenderTarget);

        for (uint32_t l_Mip = 0; l_Mip < mipCount; ++l_Mip)
        {
            uint32_t l_Size = baseSize >> l_Mip;
            if (l_Size == 0)
            {
                l_Size = 1;
            }

            float l_Roughness = mipCount > 1 ? static_cast<float>(l_Mip) / static_cast<float>(mipCount - 1) : 0.0f;

            for (uint32_t l_Face = 0; l_Face < 6; ++l_Face)
            {
                BeginFace(commandList, prefiltered, l_Mip, l_Face, l_Size, true);

                FacePush l_Push;
                l_Push.Forward = glm::vec4(k_Faces[l_Face].Forward, l_Roughness);
                l_Push.Right = glm::vec4(k_Faces[l_Face].Right, 0.0f);
                l_Push.Up = glm::vec4(k_Faces[l_Face].Up, 0.0f);

                commandList.BindPipeline(m_PrefilterPipeline);
                commandList.BindTexture(0, 0, equirect, m_EquirectSampler);
                commandList.PushConstants(ShaderStage::Vertex | ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(FacePush)), &l_Push);
                commandList.Draw(3, 1, 0, 0);

                commandList.EndRendering();
            }
        }

        commandList.TransitionTexture(prefiltered, ResourceState::RenderTarget, ResourceState::ShaderResource);
    }

    void IBLProcessor::GenerateBrdfLut(CommandList& commandList, TextureHandle brdfLut, uint32_t size)
    {
        commandList.TransitionTexture(brdfLut, ResourceState::Undefined, ResourceState::RenderTarget);

        RenderingAttachment l_Color;
        l_Color.Target = brdfLut;
        l_Color.Clear = true;

        RenderingInfo l_Info;
        l_Info.ColorAttachments = &l_Color;
        l_Info.ColorAttachmentCount = 1;
        l_Info.Depth = nullptr;
        l_Info.Width = size;
        l_Info.Height = size;

        commandList.BeginRendering(l_Info);
        SetFullViewport(commandList, size);
        commandList.BindPipeline(m_BrdfPipeline);
        commandList.Draw(3, 1, 0, 0);
        commandList.EndRendering();

        commandList.TransitionTexture(brdfLut, ResourceState::RenderTarget, ResourceState::ShaderResource);
    }

    void IBLProcessor::ClearCube(CommandList& commandList, TextureHandle target, uint32_t baseSize, uint32_t mipCount)
    {
        commandList.TransitionTexture(target, ResourceState::Undefined, ResourceState::RenderTarget);

        for (uint32_t l_Mip = 0; l_Mip < mipCount; ++l_Mip)
        {
            uint32_t l_Size = baseSize >> l_Mip;
            if (l_Size == 0)
            {
                l_Size = 1;
            }

            for (uint32_t l_Face = 0; l_Face < 6; ++l_Face)
            {
                BeginFace(commandList, target, l_Mip, l_Face, l_Size, true);
                commandList.EndRendering();
            }
        }

        commandList.TransitionTexture(target, ResourceState::RenderTarget, ResourceState::ShaderResource);
    }
}