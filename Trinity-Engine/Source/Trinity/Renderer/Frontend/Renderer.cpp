#include <Trinity/Renderer/Frontend/Renderer.h>
#include <Trinity/ImGui/ImGuiLayer.h>
#include <Trinity/ImGui/IImGuiRenderBackend.h>

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Core/Log.h>

#include <Trinity/Renderer/Meshes/Mesh.h>
#include <Trinity/Renderer/Materials/Material.h>
#include <Trinity/Renderer/Textures/Image.h>
#include <Trinity/Renderer/Materials/MaterialParameter.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Scene/Components/LightComponent.h>
#include <Trinity/Assets/AssetDatabase.h>

namespace Trinity
{
    static constexpr uint32_t k_MaxLights = 16;

    struct MeshPushConstants
    {
        glm::mat4 Model;
        glm::vec4 BaseColorFactor;
        glm::vec4 PbrFactors;      // x = metallic, y = roughness, z = occlusionStrength, w = normalScale
        glm::vec4 EmissiveFactor;  // rgb = emissive color, a = emissive strength
    };

    // Matches the std140 layout of FrameData / GpuLight in Mesh.slang.
    struct GpuLight
    {
        glm::vec4 PositionType;
        glm::vec4 DirectionRange;
        glm::vec4 ColorIntensity;
        glm::vec4 SpotAngles;
    };

    struct FrameData
    {
        glm::mat4 ViewProjection;
        glm::vec4 CameraPosition;
        glm::vec4 AmbientAndCount;
        glm::vec4 IblParams;            // x = IBL enabled, y = max prefilter LOD
        glm::mat4 LightViewProjection;  // directional shadow caster
        glm::vec4 ShadowParams;         // x = enabled, y = bias, w = 1 / shadow map size
        GpuLight Lights[k_MaxLights];
    };

    static void LogShaderDiagnostics(const char* stage, const ShaderCompileResult& result)
    {
        for (const ShaderDiagnostic& l_Message : result.Messages)
        {
            const char* l_Severity = l_Message.Severity == ShaderDiagnosticSeverity::Error ? "error" : (l_Message.Severity == ShaderDiagnosticSeverity::Warning ? "warning" : "info");
            TR_CORE_ERROR("Renderer: [{}] {}({},{}) {} {}: {}", stage, l_Message.File, l_Message.Line, l_Message.Column, l_Severity, l_Message.Code, l_Message.Message);
        }

        if (result.Messages.empty() && !result.Diagnostics.empty())
        {
            TR_CORE_ERROR("Renderer: [{}] {}", stage, result.Diagnostics);
        }
    }

    Renderer::Renderer(GraphicsDevice& device, Swapchain& swapchain, FileSystem& fileSystem) : m_Device(device), m_Swapchain(swapchain), m_FileSystem(fileSystem), m_TextureManager(device, fileSystem), m_MeshLibrary(device, fileSystem)
    {

    }

    Renderer::~Renderer()
    {
        Shutdown();
    }

    bool Renderer::Initialize()
    {
        if (!m_ShaderCompiler.Initialize())
        {
            return false;
        }

        m_ShaderCompiler.SetCacheDirectory(m_FileSystem.Resolve(BaseDirectory::UserCache, "Shaders"));

        uint32_t l_FramesInFlight = m_Swapchain.GetFramesInFlight();

        m_CommandLists.reserve(l_FramesInFlight);
        for (uint32_t l_Index = 0; l_Index < l_FramesInFlight; ++l_Index)
        {
            m_CommandLists.push_back(m_Device.CreateCommandList());
        }

        m_FrameUniforms.reserve(l_FramesInFlight);
        for (uint32_t l_Index = 0; l_Index < l_FramesInFlight; ++l_Index)
        {
            BufferDescription l_FrameDescription;
            l_FrameDescription.Size = sizeof(FrameData);
            l_FrameDescription.Usage = BufferUsage::Uniform;
            l_FrameDescription.Memory = MemoryUsage::CpuToGpu;
            l_FrameDescription.DebugName = "FrameUniform";

            BufferHandle l_Frame = m_Device.CreateBuffer(l_FrameDescription);
            if (!l_Frame.IsValid())
            {
                TR_CORE_ERROR("Renderer: frame uniform buffer creation failed");

                return false;
            }

            m_FrameUniforms.push_back(l_Frame);
        }

        if (!CreatePipeline())
        {
            return false;
        }

        if (!m_MeshLibrary.Initialize())
        {
            return false;
        }

        if (!CreateTextureResources())
        {
            return false;
        }

        if (!CreateSceneTargets(m_Swapchain.GetWidth(), m_Swapchain.GetHeight()))
        {
            return false;
        }

        std::filesystem::path l_ShaderDirectory = m_FileSystem.Resolve(BaseDirectory::Executable, "Shaders");
        if (!m_PostProcess.Initialize(m_Device, m_ShaderCompiler, l_ShaderDirectory, m_Swapchain.GetFormat()))
        {
            return false;
        }

        if (!m_DepthVisualizeStage.Initialize(m_Device, m_ShaderCompiler, l_ShaderDirectory, Format::RGBA8_UNORM))
        {
            return false;
        }

        if (!m_SkyboxStage.Initialize(m_Device, m_ShaderCompiler, l_ShaderDirectory, Format::RGBA16_SFLOAT, Format::D32_SFLOAT))
        {
            return false;
        }

        if (!m_IBLProcessor.Initialize(m_Device, m_ShaderCompiler, l_ShaderDirectory))
        {
            return false;
        }

        LoadEnvironmentMap();

        if (!CreateIBLResources())
        {
            return false;
        }

        if (!CreateShadowResources())
        {
            return false;
        }

        m_Timer.Reset();

        m_ShaderSourcePath = m_FileSystem.Resolve(BaseDirectory::Executable, "Shaders/Mesh.slang");
        std::error_code l_TimeError;
        m_ShaderWriteTime = std::filesystem::last_write_time(m_ShaderSourcePath, l_TimeError);

        TR_CORE_INFO("Renderer: initialized");
        return true;
    }

    void Renderer::Shutdown()
    {
        m_CommandLists.clear();

        for (BufferHandle& it_Frame : m_FrameUniforms)
        {
            if (it_Frame.IsValid())
            {
                m_Device.DestroyBuffer(it_Frame);
            }
        }
        m_FrameUniforms.clear();

        m_PostProcess.Shutdown();
        m_DepthVisualizeStage.Shutdown();
        m_SkyboxStage.Shutdown();
        m_IBLProcessor.Shutdown();

        if (m_EnvironmentMap.IsValid())
        {
            m_Device.DestroyTexture(m_EnvironmentMap);
            m_EnvironmentMap = TextureHandle{};
        }

        for (TextureHandle* it_Texture : { &m_IrradianceMap, &m_PrefilteredMap, &m_BrdfLut })
        {
            if (it_Texture->IsValid())
            {
                m_Device.DestroyTexture(*it_Texture);
                *it_Texture = TextureHandle{};
            }
        }

        if (m_IblCubeSampler.IsValid())
        { 
            m_Device.DestroySampler(m_IblCubeSampler);
            m_IblCubeSampler = SamplerHandle{};
        }

        if (m_BrdfSampler.IsValid())
        {
            m_Device.DestroySampler(m_BrdfSampler);
            m_BrdfSampler = SamplerHandle{};
        }

        if (m_ShadowPipeline.IsValid())
        {
            m_Device.DestroyPipeline(m_ShadowPipeline);
            m_ShadowPipeline = PipelineHandle{};
        }
        
        if (m_ShadowVertex.IsValid())
        {
            m_Device.DestroyShader(m_ShadowVertex);
            m_ShadowVertex = ShaderHandle{};
        }

        if (m_ShadowFragment.IsValid())
        {
            m_Device.DestroyShader(m_ShadowFragment);
            m_ShadowFragment = ShaderHandle{};
        }

        if (m_ShadowSampler.IsValid())
        {
            m_Device.DestroySampler(m_ShadowSampler);
            m_ShadowSampler = SamplerHandle{};
        }

        if (m_ShadowMap.IsValid())
        {
            m_Device.DestroyTexture(m_ShadowMap);
            m_ShadowMap = TextureHandle{};
        }

        if (m_Pipeline.IsValid())
        {
            m_Device.DestroyPipeline(m_Pipeline);
            m_Pipeline = PipelineHandle{};
        }

        if (m_VertexShader.IsValid())
        {
            m_Device.DestroyShader(m_VertexShader);
            m_VertexShader = ShaderHandle{};
        }

        if (m_FragmentShader.IsValid())
        {
            m_Device.DestroyShader(m_FragmentShader);
            m_FragmentShader = ShaderHandle{};
        }

        m_MeshLibrary.Shutdown();

        DestroySceneTargets();
        DestroyViewportOutput();

        m_TextureManager.Shutdown();

        m_ShaderCompiler.Shutdown();
    }

    bool Renderer::CreatePipeline()
    {
        return BuildPipeline(m_VertexShader, m_FragmentShader, m_Pipeline);
    }

    bool Renderer::BuildPipeline(ShaderHandle& vertexShader, ShaderHandle& fragmentShader, PipelineHandle& pipeline)
    {
        vertexShader = ShaderHandle{};
        fragmentShader = ShaderHandle{};
        pipeline = PipelineHandle{};

        std::filesystem::path l_ShaderDirectory = m_FileSystem.Resolve(BaseDirectory::Executable, "Shaders");

        ShaderCompileResult l_VertexResult = m_ShaderCompiler.Compile(l_ShaderDirectory, "Mesh", "vertexMain");
        if (!l_VertexResult.Success)
        {
            LogShaderDiagnostics("vertexMain", l_VertexResult);

            return false;
        }

        ShaderCompileResult l_FragmentResult = m_ShaderCompiler.Compile(l_ShaderDirectory, "Mesh", "fragmentMain");
        if (!l_FragmentResult.Success)
        {
            LogShaderDiagnostics("fragmentMain", l_FragmentResult);

            return false;
        }

        TR_CORE_INFO("Renderer: reflection - push constant {} bytes, {} vertex inputs", l_VertexResult.Reflection.PushConstantSize, l_VertexResult.Reflection.VertexInputs.size());
        for (const ShaderVertexInput& it_Input : l_VertexResult.Reflection.VertexInputs)
        {
            TR_CORE_INFO("Renderer:   input '{}' @ location {}", it_Input.Name, it_Input.Location);
        }

        ShaderDescription l_VertexDescription;
        l_VertexDescription.Stage = ShaderStage::Vertex;
        l_VertexDescription.Bytecode = l_VertexResult.SPIRV;
        l_VertexDescription.EntryPoint = "vertexMain";
        l_VertexDescription.DebugName = "Mesh.vertexMain";
        vertexShader = m_Device.CreateShader(l_VertexDescription);

        ShaderDescription l_FragmentDescription;
        l_FragmentDescription.Stage = ShaderStage::Fragment;
        l_FragmentDescription.Bytecode = l_FragmentResult.SPIRV;
        l_FragmentDescription.EntryPoint = "fragmentMain";
        l_FragmentDescription.DebugName = "Mesh.fragmentMain";
        fragmentShader = m_Device.CreateShader(l_FragmentDescription);

        if (!vertexShader.IsValid() || !fragmentShader.IsValid())
        {
            TR_CORE_ERROR("Renderer: shader module creation failed");
            if (vertexShader.IsValid())
            {
                m_Device.DestroyShader(vertexShader); vertexShader = ShaderHandle{};
            }

            if (fragmentShader.IsValid())
            {
                m_Device.DestroyShader(fragmentShader); fragmentShader = ShaderHandle{};
            }

            return false;
        }

        PipelineDescription l_PipelineDescription;
        l_PipelineDescription.VertexShader = vertexShader;
        l_PipelineDescription.FragmentShader = fragmentShader;
        l_PipelineDescription.Vertex = MeshVertex::GetLayout();
        l_PipelineDescription.Topology = PrimitiveTopology::TriangleList;
        l_PipelineDescription.Rasterizer.Cull = CullMode::None;
        l_PipelineDescription.DepthStencil.DepthTest = true;
        l_PipelineDescription.DepthStencil.DepthWrite = true;
        l_PipelineDescription.DepthFormat = Format::D32_SFLOAT;
        l_PipelineDescription.ColorFormats = { Format::RGBA16_SFLOAT };
        l_PipelineDescription.PushConstantSize = static_cast<uint32_t>(sizeof(MeshPushConstants));

        // The Vulkan backend allocates one descriptor set per bind call, so each resource lives in its own set.
        ResourceBinding l_FrameBinding;
        l_FrameBinding.Set = 0;
        l_FrameBinding.Binding = 0;
        l_FrameBinding.Type = ResourceBindingType::UniformBuffer;
        l_FrameBinding.Stages = ShaderStage::Vertex | ShaderStage::Fragment;

        ResourceBinding l_BaseColorBinding;
        l_BaseColorBinding.Set = 1;
        l_BaseColorBinding.Binding = 0;
        l_BaseColorBinding.Type = ResourceBindingType::CombinedImageSampler;
        l_BaseColorBinding.Stages = ShaderStage::Fragment;

        ResourceBinding l_NormalBinding;
        l_NormalBinding.Set = 2;
        l_NormalBinding.Binding = 0;
        l_NormalBinding.Type = ResourceBindingType::CombinedImageSampler;
        l_NormalBinding.Stages = ShaderStage::Fragment;

        ResourceBinding l_MetallicRoughnessBinding;
        l_MetallicRoughnessBinding.Set = 3;
        l_MetallicRoughnessBinding.Binding = 0;
        l_MetallicRoughnessBinding.Type = ResourceBindingType::CombinedImageSampler;
        l_MetallicRoughnessBinding.Stages = ShaderStage::Fragment;

        ResourceBinding l_EmissiveBinding;
        l_EmissiveBinding.Set = 4;
        l_EmissiveBinding.Binding = 0;
        l_EmissiveBinding.Type = ResourceBindingType::CombinedImageSampler;
        l_EmissiveBinding.Stages = ShaderStage::Fragment;

        ResourceBinding l_IrradianceBinding;
        l_IrradianceBinding.Set = 5;
        l_IrradianceBinding.Binding = 0;
        l_IrradianceBinding.Type = ResourceBindingType::CombinedImageSampler;
        l_IrradianceBinding.Stages = ShaderStage::Fragment;

        ResourceBinding l_PrefilteredBinding;
        l_PrefilteredBinding.Set = 6;
        l_PrefilteredBinding.Binding = 0;
        l_PrefilteredBinding.Type = ResourceBindingType::CombinedImageSampler;
        l_PrefilteredBinding.Stages = ShaderStage::Fragment;

        ResourceBinding l_BrdfBinding;
        l_BrdfBinding.Set = 7;
        l_BrdfBinding.Binding = 0;
        l_BrdfBinding.Type = ResourceBindingType::CombinedImageSampler;
        l_BrdfBinding.Stages = ShaderStage::Fragment;

        ResourceBinding l_ShadowBinding;
        l_ShadowBinding.Set = 8;
        l_ShadowBinding.Binding = 0;
        l_ShadowBinding.Type = ResourceBindingType::CombinedImageSampler;
        l_ShadowBinding.Stages = ShaderStage::Fragment;

        l_PipelineDescription.Bindings = { l_FrameBinding, l_BaseColorBinding, l_NormalBinding, l_MetallicRoughnessBinding, l_EmissiveBinding, l_IrradianceBinding, l_PrefilteredBinding, l_BrdfBinding, l_ShadowBinding };
        l_PipelineDescription.DebugName = "Mesh";

        pipeline = m_Device.CreatePipeline(l_PipelineDescription);
        if (!pipeline.IsValid())
        {
            TR_CORE_ERROR("Renderer: pipeline creation failed");
            m_Device.DestroyShader(vertexShader); vertexShader = ShaderHandle{};
            m_Device.DestroyShader(fragmentShader); fragmentShader = ShaderHandle{};

            return false;
        }

        return true;
    }

    void Renderer::ReloadShaders()
    {
        TR_CORE_INFO("Renderer: shader change detected, recompiling");

        ShaderHandle l_NewVertex;
        ShaderHandle l_NewFragment;
        PipelineHandle l_NewPipeline;

        if (!BuildPipeline(l_NewVertex, l_NewFragment, l_NewPipeline))
        {
            TR_CORE_WARN("Renderer: shader reload failed, keeping previous pipeline");
            return;
        }

        m_Device.WaitIdle();

        if (m_Pipeline.IsValid())
        {
            m_Device.DestroyPipeline(m_Pipeline);
        }

        if (m_VertexShader.IsValid())
        {
            m_Device.DestroyShader(m_VertexShader);
        }

        if (m_FragmentShader.IsValid())
        {
            m_Device.DestroyShader(m_FragmentShader);
        }

        m_VertexShader = l_NewVertex;
        m_FragmentShader = l_NewFragment;
        m_Pipeline = l_NewPipeline;

        TR_CORE_INFO("Renderer: shader reload complete");
    }

    void Renderer::CheckHotReload()
    {
        if (m_ShaderSourcePath.empty())
        {
            return;
        }

        std::error_code l_Error;
        std::filesystem::file_time_type l_Current = std::filesystem::last_write_time(m_ShaderSourcePath, l_Error);
        if (l_Error)
        {
            return;
        }

        if (l_Current != m_ShaderWriteTime)
        {
            m_ShaderWriteTime = l_Current;
            ReloadShaders();
        }
    }

    bool Renderer::CreateTextureResources()
    {
        return m_TextureManager.Initialize();
    }

    bool Renderer::CreateSceneTargets(uint32_t width, uint32_t height)
    {
        TextureDescription l_ColorDescription;
        l_ColorDescription.Type = TextureType::Texture2D;
        l_ColorDescription.Format = Format::RGBA16_SFLOAT;
        l_ColorDescription.Usage = TextureUsage::Sampled | TextureUsage::RenderTarget;
        l_ColorDescription.Width = width;
        l_ColorDescription.Height = height;
        l_ColorDescription.Depth = 1;
        l_ColorDescription.MipLevels = 1;
        l_ColorDescription.ArrayLayers = 1;
        l_ColorDescription.SampleCount = 1;
        l_ColorDescription.DebugName = "SceneColor";

        m_SceneColor = m_Device.CreateTexture(l_ColorDescription);
        if (!m_SceneColor.IsValid())
        {
            TR_CORE_ERROR("Renderer: scene color target creation failed");

            return false;
        }

        TextureDescription l_DepthDescription;
        l_DepthDescription.Type = TextureType::Texture2D;
        l_DepthDescription.Format = Format::D32_SFLOAT;
        l_DepthDescription.Usage = TextureUsage::DepthStencil | TextureUsage::Sampled;
        l_DepthDescription.Width = width;
        l_DepthDescription.Height = height;
        l_DepthDescription.Depth = 1;
        l_DepthDescription.MipLevels = 1;
        l_DepthDescription.ArrayLayers = 1;
        l_DepthDescription.SampleCount = 1;
        l_DepthDescription.DebugName = "SceneDepth";

        m_SceneDepth = m_Device.CreateTexture(l_DepthDescription);
        if (!m_SceneDepth.IsValid())
        {
            TR_CORE_ERROR("Renderer: scene depth target creation failed");

            return false;
        }

        TextureDescription l_DepthVisDescription;
        l_DepthVisDescription.Type = TextureType::Texture2D;
        l_DepthVisDescription.Format = Format::RGBA8_UNORM;
        l_DepthVisDescription.Usage = TextureUsage::Sampled | TextureUsage::RenderTarget;
        l_DepthVisDescription.Width = width;
        l_DepthVisDescription.Height = height;
        l_DepthVisDescription.Depth = 1;
        l_DepthVisDescription.MipLevels = 1;
        l_DepthVisDescription.ArrayLayers = 1;
        l_DepthVisDescription.SampleCount = 1;
        l_DepthVisDescription.DebugName = "DepthVis";

        m_DepthVis = m_Device.CreateTexture(l_DepthVisDescription);
        if (!m_DepthVis.IsValid())
        {
            TR_CORE_ERROR("Renderer: depth visualization target creation failed");

            return false;
        }

        m_RenderWidth = width;
        m_RenderHeight = height;

        return true;
    }

    void Renderer::DestroySceneTargets()
    {
        if (m_SceneColor.IsValid())
        {
            m_Device.DestroyTexture(m_SceneColor);
            m_SceneColor = TextureHandle{};
        }

        if (m_SceneDepth.IsValid())
        {
            m_Device.DestroyTexture(m_SceneDepth);
            m_SceneDepth = TextureHandle{};
        }

        if (m_DepthVis.IsValid())
        {
            m_Device.DestroyTexture(m_DepthVis);
            m_DepthVis = TextureHandle{};
        }

        m_RenderWidth = 0;
        m_RenderHeight = 0;
    }

    void Renderer::SetViewportSize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        m_ViewportActive = true;

        if (width != m_RenderWidth || height != m_RenderHeight)
        {
            m_PendingWidth = width;
            m_PendingHeight = height;
            m_ViewportDirty = true;
        }
    }

    void Renderer::ApplyViewportResize()
    {
        if (!m_ViewportDirty)
        {
            return;
        }

        m_Device.WaitIdle();
        DestroyViewportOutput();
        DestroySceneTargets();
        CreateSceneTargets(m_PendingWidth, m_PendingHeight);
        CreateViewportOutput(m_PendingWidth, m_PendingHeight);
        m_ViewportDirty = false;
    }

    bool Renderer::CreateViewportOutput(uint32_t width, uint32_t height)
    {
        TextureDescription l_Description;
        l_Description.Type = TextureType::Texture2D;
        l_Description.Format = m_Swapchain.GetFormat();
        l_Description.Usage = TextureUsage::Sampled | TextureUsage::RenderTarget;
        l_Description.Width = width;
        l_Description.Height = height;
        l_Description.Depth = 1;
        l_Description.MipLevels = 1;
        l_Description.ArrayLayers = 1;
        l_Description.SampleCount = 1;
        l_Description.DebugName = "ViewportColor";

        m_ViewportColor = m_Device.CreateTexture(l_Description);
        if (!m_ViewportColor.IsValid())
        {
            TR_CORE_ERROR("Renderer: viewport output creation failed");

            return false;
        }

        m_ViewportTextureID = m_Device.GetImGuiBackend().RegisterTexture(m_ViewportColor);

        return true;
    }

    void Renderer::DestroyViewportOutput()
    {
        if (m_ViewportTextureID != 0)
        {
            m_Device.GetImGuiBackend().UnregisterTexture(m_ViewportTextureID);
            m_ViewportTextureID = 0;
        }

        if (m_ViewportColor.IsValid())
        {
            m_Device.DestroyTexture(m_ViewportColor);
            m_ViewportColor = TextureHandle{};
        }
    }

    void Renderer::LoadEnvironmentMap()
    {
        std::filesystem::path l_Path = m_FileSystem.Resolve(BaseDirectory::Executable, "Assets/Environment.hdr");

        std::error_code l_Error;
        if (!std::filesystem::exists(l_Path, l_Error))
        {
            TR_CORE_WARN("Renderer: no environment map at '{}'; skybox disabled", l_Path.string());

            return;
        }

        std::optional<ImageData> l_Image = Image::Load(l_Path, false);
        if (!l_Image)
        {
            return;
        }

        TextureDescription l_Description;
        l_Description.Type = TextureType::Texture2D;
        l_Description.Format = l_Image->SuggestedFormat;
        l_Description.Usage = TextureUsage::Sampled;
        l_Description.Width = l_Image->Width;
        l_Description.Height = l_Image->Height;
        l_Description.Depth = 1;
        l_Description.MipLevels = 1;
        l_Description.ArrayLayers = 1;
        l_Description.SampleCount = 1;
        l_Description.InitialData = l_Image->Pixels.data();
        l_Description.InitialDataSize = l_Image->Pixels.size();
        l_Description.DebugName = "EnvironmentMap";

        m_EnvironmentMap = m_Device.CreateTexture(l_Description);
        if (!m_EnvironmentMap.IsValid())
        {
            TR_CORE_ERROR("Renderer: environment map upload failed");
        }
    }

    bool Renderer::CreateIBLResources()
    {
        auto l_MakeCube = [this](uint32_t size, uint32_t mips, const char* name) -> TextureHandle
            {
                TextureDescription l_Description;
                l_Description.Type = TextureType::TextureCube;
                l_Description.Format = Format::RGBA16_SFLOAT;
                l_Description.Usage = TextureUsage::Sampled | TextureUsage::RenderTarget;
                l_Description.Width = size;
                l_Description.Height = size;
                l_Description.Depth = 1;
                l_Description.MipLevels = mips;
                l_Description.ArrayLayers = 1;
                l_Description.SampleCount = 1;
                l_Description.DebugName = name;

                return m_Device.CreateTexture(l_Description);
            };

        m_IrradianceMap = l_MakeCube(k_IrradianceSize, 1, "IrradianceMap");
        m_PrefilteredMap = l_MakeCube(k_PrefilterSize, k_PrefilterMips, "PrefilteredMap");

        TextureDescription l_LutDescription;
        l_LutDescription.Type = TextureType::Texture2D;
        l_LutDescription.Format = Format::RG16_SFLOAT;
        l_LutDescription.Usage = TextureUsage::Sampled | TextureUsage::RenderTarget;
        l_LutDescription.Width = k_BrdfLutSize;
        l_LutDescription.Height = k_BrdfLutSize;
        l_LutDescription.Depth = 1;
        l_LutDescription.MipLevels = 1;
        l_LutDescription.ArrayLayers = 1;
        l_LutDescription.SampleCount = 1;
        l_LutDescription.DebugName = "BrdfLut";
        m_BrdfLut = m_Device.CreateTexture(l_LutDescription);

        if (!m_IrradianceMap.IsValid() || !m_PrefilteredMap.IsValid() || !m_BrdfLut.IsValid())
        {
            TR_CORE_ERROR("Renderer: IBL resource creation failed");

            return false;
        }

        SamplerDescription l_CubeSampler;
        l_CubeSampler.LinearFilter = true;
        l_CubeSampler.LinearMipmap = true;
        l_CubeSampler.RepeatU = false;
        l_CubeSampler.RepeatV = false;
        l_CubeSampler.RepeatW = false;
        l_CubeSampler.DebugName = "IBLCubeSampler";
        m_IblCubeSampler = m_Device.CreateSampler(l_CubeSampler);

        SamplerDescription l_LutSampler;
        l_LutSampler.LinearFilter = true;
        l_LutSampler.LinearMipmap = false;
        l_LutSampler.RepeatU = false;
        l_LutSampler.RepeatV = false;
        l_LutSampler.RepeatW = false;
        l_LutSampler.DebugName = "BrdfLutSampler";
        m_BrdfSampler = m_Device.CreateSampler(l_LutSampler);

        return m_IblCubeSampler.IsValid() && m_BrdfSampler.IsValid();
    }

    std::vector<DebugRenderTarget> Renderer::GetDebugRenderTargets() const
    {
        std::vector<DebugRenderTarget> l_Targets;

        if (m_SceneColor.IsValid())
        {
            l_Targets.push_back({ "SceneColor", m_SceneColor, m_RenderWidth, m_RenderHeight });
        }

        if (m_ViewportColor.IsValid())
        {
            l_Targets.push_back({ "ViewportColor", m_ViewportColor, m_RenderWidth, m_RenderHeight });
        }

        if (m_DepthVisualize && m_DepthVis.IsValid())
        {
            l_Targets.push_back({ "DepthVis", m_DepthVis, m_RenderWidth, m_RenderHeight });
        }

        return l_Targets;
    }

    bool Renderer::CreateShadowResources()
    {
        TextureDescription l_Description;
        l_Description.Type = TextureType::Texture2D;
        l_Description.Format = Format::D32_SFLOAT;
        l_Description.Usage = TextureUsage::DepthStencil | TextureUsage::Sampled;
        l_Description.Width = k_ShadowMapSize;
        l_Description.Height = k_ShadowMapSize;
        l_Description.Depth = 1;
        l_Description.MipLevels = 1;
        l_Description.ArrayLayers = 1;
        l_Description.SampleCount = 1;
        l_Description.DebugName = "ShadowMap";

        m_ShadowMap = m_Device.CreateTexture(l_Description);
        if (!m_ShadowMap.IsValid())
        {
            TR_CORE_ERROR("Renderer: shadow map creation failed");

            return false;
        }

        SamplerDescription l_SamplerDescription;
        l_SamplerDescription.LinearFilter = false;
        l_SamplerDescription.LinearMipmap = false;
        l_SamplerDescription.RepeatU = false;
        l_SamplerDescription.RepeatV = false;
        l_SamplerDescription.RepeatW = false;
        l_SamplerDescription.DebugName = "ShadowSampler";
        m_ShadowSampler = m_Device.CreateSampler(l_SamplerDescription);

        std::filesystem::path l_ShaderDirectory = m_FileSystem.Resolve(BaseDirectory::Executable, "Shaders");

        ShaderCompileResult l_VertexResult = m_ShaderCompiler.Compile(l_ShaderDirectory, "Shadow", "vertexMain");
        ShaderCompileResult l_FragmentResult = m_ShaderCompiler.Compile(l_ShaderDirectory, "Shadow", "fragmentMain");
        if (!l_VertexResult.Success || !l_FragmentResult.Success)
        {
            TR_CORE_ERROR("Renderer: shadow shader compilation failed");

            return false;
        }

        ShaderDescription l_VertexDescription;
        l_VertexDescription.Stage = ShaderStage::Vertex;
        l_VertexDescription.Bytecode = l_VertexResult.SPIRV;
        l_VertexDescription.EntryPoint = "vertexMain";
        l_VertexDescription.DebugName = "Shadow.vertexMain";
        m_ShadowVertex = m_Device.CreateShader(l_VertexDescription);

        ShaderDescription l_FragmentDescription;
        l_FragmentDescription.Stage = ShaderStage::Fragment;
        l_FragmentDescription.Bytecode = l_FragmentResult.SPIRV;
        l_FragmentDescription.EntryPoint = "fragmentMain";
        l_FragmentDescription.DebugName = "Shadow.fragmentMain";
        m_ShadowFragment = m_Device.CreateShader(l_FragmentDescription);

        if (!m_ShadowVertex.IsValid() || !m_ShadowFragment.IsValid())
        {
            TR_CORE_ERROR("Renderer: shadow shader creation failed");

            return false;
        }

        // The shadow vertex shader only consumes Position, but the bound buffer is a full MeshVertex, so keep the stride and expose just location 0
        VertexLayout l_ShadowLayout;
        l_ShadowLayout.Stride = sizeof(MeshVertex);
        l_ShadowLayout.Attributes = { { 0, offsetof(MeshVertex, Position), Format::RGB32_SFLOAT } };

        PipelineDescription l_PipelineDescription;
        l_PipelineDescription.VertexShader = m_ShadowVertex;
        l_PipelineDescription.FragmentShader = m_ShadowFragment;
        l_PipelineDescription.Vertex = l_ShadowLayout;
        l_PipelineDescription.Topology = PrimitiveTopology::TriangleList;
        l_PipelineDescription.Rasterizer.Cull = CullMode::None;
        l_PipelineDescription.DepthStencil.DepthTest = true;
        l_PipelineDescription.DepthStencil.DepthWrite = true;
        l_PipelineDescription.DepthFormat = Format::D32_SFLOAT;
        l_PipelineDescription.PushConstantSize = static_cast<uint32_t>(sizeof(glm::mat4));
        l_PipelineDescription.DebugName = "Shadow";

        m_ShadowPipeline = m_Device.CreatePipeline(l_PipelineDescription);
        if (!m_ShadowPipeline.IsValid())
        {
            TR_CORE_ERROR("Renderer: shadow pipeline creation failed");

            return false;
        }

        return m_ShadowSampler.IsValid();
    }

    glm::mat4 Renderer::ComputeLightMatrix(const glm::vec3& direction) const
    {
        glm::vec3 l_Direction = glm::normalize(direction);

        // Up vector guarded against a near-vertical light.
        glm::vec3 l_Up = glm::abs(l_Direction.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

        const float l_Radius = 25.0f;
        glm::vec3 l_Center(0.0f);
        glm::vec3 l_Eye = l_Center - l_Direction * 50.0f;

        glm::mat4 l_View = glm::lookAt(l_Eye, l_Center, l_Up);
        glm::mat4 l_Projection = glm::ortho(-l_Radius, l_Radius, -l_Radius, l_Radius, 0.1f, 100.0f);

        return l_Projection * l_View;
    }

    bool Renderer::ComputeShadowLight(Scene& scene, glm::mat4& outMatrix)
    {
        glm::vec3 l_Direction(0.0f);
        bool l_Found = false;
        uint32_t l_LightCount = 0;

        auto l_View = scene.GetRegistry().view<TransformComponent, LightComponent>();
        for (entt::entity l_Entity : l_View)
        {
            ++l_LightCount;

            const LightComponent& l_Light = l_View.get<LightComponent>(l_Entity);
            if (!l_Found && l_Light.Type == LightType::Directional)
            {
                glm::mat4 l_World = scene.GetWorldMatrix(l_Entity);
                l_Direction = glm::normalize(glm::mat3(l_World) * glm::vec3(0.0f, 0.0f, -1.0f));
                l_Found = true;
            }
        }

        if (!l_Found)
        {
            if (l_LightCount != 0)
            {
                return false;
            }

            // Matches the default sun injected in DrawScene when the scene has no lights.
            l_Direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
        }

        outMatrix = ComputeLightMatrix(l_Direction);

        return true;
    }

    void Renderer::DrawSceneDepth(CommandList& commandList, Scene& scene, const glm::mat4& lightViewProjection)
    {
        commandList.BindPipeline(m_ShadowPipeline);

        auto l_View = scene.GetRegistry().view<TransformComponent, MeshRendererComponent>();
        for (entt::entity l_Entity : l_View)
        {
            const MeshRendererComponent& l_MeshRenderer = l_View.get<MeshRendererComponent>(l_Entity);
            if (!l_MeshRenderer.MeshReference || !l_MeshRenderer.MeshReference->IsValid())
            {
                continue;
            }

            Mesh& l_Mesh = *l_MeshRenderer.MeshReference;
            glm::mat4 l_MVP = lightViewProjection * scene.GetWorldMatrix(l_Entity);

            commandList.BindVertexBuffer(l_Mesh.GetVertexBuffer(), 0);
            commandList.BindIndexBuffer(l_Mesh.GetIndexBuffer(), 0);

            for (const Submesh& l_Submesh : l_Mesh.GetSubmeshes())
            {
                commandList.PushConstants(ShaderStage::Vertex | ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(glm::mat4)), &l_MVP);
                commandList.DrawIndexed(l_Submesh.IndexCount, 1, l_Submesh.FirstIndex, static_cast<int32_t>(l_Submesh.BaseVertex), 0);
                ++m_Stats.ShadowDrawCalls;
            }
        }
    }

    void Renderer::DrawScene(CommandList& commandList, Scene& scene, AssetDatabase& assetDatabase, const Camera& camera)
    {
        FrameData l_FrameData{};
        l_FrameData.ViewProjection = camera.GetViewProjection();
        l_FrameData.CameraPosition = glm::vec4(camera.GetPosition(), 1.0f);
        l_FrameData.AmbientAndCount = glm::vec4(0.03f, 0.03f, 0.03f, 0.0f);

        uint32_t l_LightCount = 0;
        auto l_LightView = scene.GetRegistry().view<TransformComponent, LightComponent>();
        for (entt::entity l_Entity : l_LightView)
        {
            if (l_LightCount >= k_MaxLights)
            {
                break;
            }

            const LightComponent& l_Light = l_LightView.get<LightComponent>(l_Entity);

            glm::mat4 l_World = scene.GetWorldMatrix(l_Entity);
            glm::vec3 l_Position = glm::vec3(l_World[3]);
            glm::vec3 l_Direction = glm::normalize(glm::mat3(l_World) * glm::vec3(0.0f, 0.0f, -1.0f));

            GpuLight& l_GpuLight = l_FrameData.Lights[l_LightCount];
            l_GpuLight.PositionType = glm::vec4(l_Position, static_cast<float>(l_Light.Type));
            l_GpuLight.DirectionRange = glm::vec4(l_Direction, l_Light.Range);
            l_GpuLight.ColorIntensity = glm::vec4(l_Light.Color, l_Light.Intensity);
            l_GpuLight.SpotAngles = glm::vec4(std::cos(l_Light.InnerConeAngle), std::cos(l_Light.OuterConeAngle), 0.0f, 0.0f);

            ++l_LightCount;
        }

        // Fall back to a default directional light so scenes authored before lights existed remain visible.
        if (l_LightCount == 0)
        {
            GpuLight& l_GpuLight = l_FrameData.Lights[0];
            l_GpuLight.PositionType = glm::vec4(0.0f, 0.0f, 0.0f, static_cast<float>(LightType::Directional));
            l_GpuLight.DirectionRange = glm::vec4(glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f)), 0.0f);
            l_GpuLight.ColorIntensity = glm::vec4(1.0f, 1.0f, 1.0f, 3.0f);
            l_GpuLight.SpotAngles = glm::vec4(0.0f);

            l_LightCount = 1;
        }

        l_FrameData.AmbientAndCount.a = static_cast<float>(l_LightCount);
        l_FrameData.IblParams = glm::vec4(m_EnvironmentMap.IsValid() ? 1.0f : 0.0f, static_cast<float>(k_PrefilterMips - 1), 0.0f, 0.0f);
        l_FrameData.LightViewProjection = m_ShadowLightViewProjection;
        l_FrameData.ShadowParams = glm::vec4(m_ShadowActive ? 1.0f : 0.0f, k_ShadowBias, 0.0f, 1.0f / static_cast<float>(k_ShadowMapSize));

        BufferHandle l_FrameUniform = m_FrameUniforms[m_FrameIndex];
        m_Device.UpdateBuffer(l_FrameUniform, &l_FrameData, sizeof(FrameData), 0);

        commandList.BindPipeline(m_Pipeline);
        commandList.BindUniformBuffer(0, 0, l_FrameUniform, 0, sizeof(FrameData));
        commandList.BindTexture(5, 0, m_IrradianceMap, m_IblCubeSampler);
        commandList.BindTexture(6, 0, m_PrefilteredMap, m_IblCubeSampler);
        commandList.BindTexture(7, 0, m_BrdfLut, m_BrdfSampler);
        commandList.BindTexture(8, 0, m_ShadowMap, m_ShadowSampler);

        SamplerHandle l_Sampler = m_TextureManager.DefaultSampler();

        auto l_View = scene.GetRegistry().view<TransformComponent, MeshRendererComponent>();
        for (entt::entity l_Entity : l_View)
        {
            const MeshRendererComponent& l_MeshRenderer = l_View.get<MeshRendererComponent>(l_Entity);
            if (!l_MeshRenderer.MeshReference || !l_MeshRenderer.MeshReference->IsValid())
            {
                continue;
            }

            Mesh& l_Mesh = *l_MeshRenderer.MeshReference;
            const std::vector<MaterialSlot>& l_Slots = l_Mesh.GetMaterialSlots();
            ++m_Stats.Meshes;

            glm::mat4 l_Model = scene.GetWorldMatrix(l_Entity);

            commandList.BindVertexBuffer(l_Mesh.GetVertexBuffer(), 0);
            commandList.BindIndexBuffer(l_Mesh.GetIndexBuffer(), 0);

            for (const Submesh& l_Submesh : l_Mesh.GetSubmeshes())
            {
                MeshPushConstants l_PushConstants;
                l_PushConstants.Model = l_Model;
                l_PushConstants.BaseColorFactor = glm::vec4(1.0f);
                l_PushConstants.PbrFactors = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
                l_PushConstants.EmissiveFactor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

                TextureHandle l_BaseColor = m_TextureManager.White();
                TextureHandle l_Normal = m_TextureManager.Normal();
                TextureHandle l_MetallicRoughness = m_TextureManager.White();
                TextureHandle l_Emissive = m_TextureManager.White();

                UUID l_MaterialAsset = l_Submesh.MaterialIndex < l_MeshRenderer.Materials.size() ? l_MeshRenderer.Materials[l_Submesh.MaterialIndex] : UUID(0);
                if (static_cast<uint64_t>(l_MaterialAsset) != 0)
                {
                    std::shared_ptr<Material> l_Material = assetDatabase.ResolveMaterial(l_MaterialAsset);
                    if (l_Material)
                    {
                        if (const MaterialParameter* l_Factor = l_Material->FindParameter(Material::BaseColorFactor))
                        {
                            l_PushConstants.BaseColorFactor = l_Factor->AsVec4();
                        }

                        if (const MaterialParameter* l_Metallic = l_Material->FindParameter(Material::MetallicFactor))
                        {
                            l_PushConstants.PbrFactors.x = l_Metallic->AsFloat();
                        }

                        if (const MaterialParameter* l_Roughness = l_Material->FindParameter(Material::RoughnessFactor))
                        {
                            l_PushConstants.PbrFactors.y = l_Roughness->AsFloat();
                        }

                        if (const MaterialParameter* l_Occlusion = l_Material->FindParameter(Material::OcclusionStrength))
                        {
                            l_PushConstants.PbrFactors.z = l_Occlusion->AsFloat();
                        }

                        if (const MaterialParameter* l_NormalScale = l_Material->FindParameter(Material::NormalScale))
                        {
                            l_PushConstants.PbrFactors.w = l_NormalScale->AsFloat();
                        }

                        if (const MaterialParameter* l_Emissive = l_Material->FindParameter(Material::EmissiveFactor))
                        {
                            l_PushConstants.EmissiveFactor = glm::vec4(l_Emissive->AsVec3(), l_PushConstants.EmissiveFactor.a);
                        }

                        if (const MaterialParameter* l_EmissiveStrength = l_Material->FindParameter(Material::EmissiveStrength))
                        {
                            l_PushConstants.EmissiveFactor.a = l_EmissiveStrength->AsFloat();
                        }

                        if (const MaterialParameter* l_Texture = l_Material->FindParameter(Material::BaseColorTexture); l_Texture != nullptr && static_cast<uint64_t>(l_Texture->AsTexture()) != 0)
                        {
                            l_BaseColor = assetDatabase.ResolveTexture(l_Texture->AsTexture());
                        }

                        if (const MaterialParameter* l_Texture = l_Material->FindParameter(Material::NormalTexture); l_Texture != nullptr && static_cast<uint64_t>(l_Texture->AsTexture()) != 0)
                        {
                            l_Normal = assetDatabase.ResolveTexture(l_Texture->AsTexture());
                        }

                        if (const MaterialParameter* l_Texture = l_Material->FindParameter(Material::MetallicRoughnessTexture); l_Texture != nullptr && static_cast<uint64_t>(l_Texture->AsTexture()) != 0)
                        {
                            l_MetallicRoughness = assetDatabase.ResolveTexture(l_Texture->AsTexture());
                        }

                        if (const MaterialParameter* l_Texture = l_Material->FindParameter(Material::EmissiveTexture); l_Texture != nullptr && static_cast<uint64_t>(l_Texture->AsTexture()) != 0)
                        {
                            l_Emissive = assetDatabase.ResolveTexture(l_Texture->AsTexture());
                        }
                    }
                }
                else if (l_Submesh.MaterialIndex < l_Slots.size())
                {
                    l_PushConstants.BaseColorFactor = l_Slots[l_Submesh.MaterialIndex].BaseColorFactor;
                }

                commandList.BindTexture(1, 0, l_BaseColor, l_Sampler);
                commandList.BindTexture(2, 0, l_Normal, l_Sampler);
                commandList.BindTexture(3, 0, l_MetallicRoughness, l_Sampler);
                commandList.BindTexture(4, 0, l_Emissive, l_Sampler);
                commandList.PushConstants(ShaderStage::Vertex | ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(l_PushConstants)), &l_PushConstants);

                commandList.DrawIndexed(l_Submesh.IndexCount, 1, l_Submesh.FirstIndex, static_cast<int32_t>(l_Submesh.BaseVertex), 0);
                ++m_Stats.DrawCalls;
                m_Stats.Triangles += l_Submesh.IndexCount / 3;
            }
        }
    }

    void Renderer::RenderFrame(Scene& scene, AssetDatabase& assetDatabase, const Camera& camera, ImGuiLayer* imgui)
    {
        m_Device.CollectGarbage();

        if (m_Minimized)
        {
            return;
        }

        m_Stats = RenderStats{};

        float l_Elapsed = m_Timer.Elapsed();
        if (l_Elapsed - m_LastReloadCheck >= 0.5f)
        {
            m_LastReloadCheck = l_Elapsed;
            CheckHotReload();
        }

        if (!m_SceneColor.IsValid() || !m_SceneDepth.IsValid())
        {
            return;
        }

        bool l_UseViewport = m_ViewportActive && m_ViewportColor.IsValid();

        FrameInfo l_Frame;
        if (!m_Swapchain.AcquireNextImage(l_Frame))
        {
            return;
        }

        CommandList& l_CommandList = *m_CommandLists[m_FrameIndex];

        uint32_t l_SwapWidth = m_Swapchain.GetWidth();
        uint32_t l_SwapHeight = m_Swapchain.GetHeight();

        // Fresh per-frame targets start in Undefined (contents discarded).
        m_RenderGraph.Reset();
        m_RenderGraph.Import(m_SceneColor, ResourceState::Undefined, "SceneColor");
        m_RenderGraph.Import(m_SceneDepth, ResourceState::Undefined, "SceneDepth");
        m_RenderGraph.Import(l_Frame.BackBuffer, ResourceState::Undefined, "BackBuffer");
        if (l_UseViewport)
        {
            m_RenderGraph.Import(m_ViewportColor, ResourceState::Undefined, "ViewportColor");
        }

        // Directional shadow map (depth-only). Always recorded so the map stays valid to sample.
        m_ShadowActive = ComputeShadowLight(scene, m_ShadowLightViewProjection);
        m_RenderGraph.Import(m_ShadowMap, ResourceState::Undefined, "ShadowMap");
        {
            RenderGraphPass& l_Pass = m_RenderGraph.AddPass("Shadow");
            l_Pass.Depth = m_ShadowMap;
            l_Pass.ClearDepth = true;
            l_Pass.DepthClearValue = 1.0f;
            l_Pass.Width = k_ShadowMapSize;
            l_Pass.Height = k_ShadowMapSize;
            l_Pass.ManageRendering = true;

            glm::mat4 l_LightViewProjection = m_ShadowLightViewProjection;
            bool l_Active = m_ShadowActive;
            l_Pass.Execute = [this, &scene, l_LightViewProjection, l_Active](CommandList& commandList)
                {
                    if (l_Active)
                    {
                        DrawSceneDepth(commandList, scene, l_LightViewProjection);
                    }
                };
        }

        // Scene pass: draw the lit scene into the HDR color target.
        {
            RenderGraphPass& l_Pass = m_RenderGraph.AddPass("Scene");
            l_Pass.Reads.push_back(m_ShadowMap);

            RenderGraphColorTarget l_Color;
            l_Color.Target = m_SceneColor;
            l_Color.Clear = true;
            l_Color.ClearColor[0] = 0.0f;
            l_Color.ClearColor[1] = 0.0f;
            l_Color.ClearColor[2] = 0.0f;
            l_Color.ClearColor[3] = 1.0f;
            l_Pass.Colors.push_back(l_Color);

            l_Pass.Depth = m_SceneDepth;
            l_Pass.ClearDepth = true;
            l_Pass.DepthClearValue = 1.0f;
            l_Pass.Width = m_RenderWidth;
            l_Pass.Height = m_RenderHeight;
            l_Pass.ManageRendering = true;
            l_Pass.Execute = [this, &scene, &assetDatabase, &camera](CommandList& commandList)
                {
                    if (m_EnvironmentMap.IsValid())
                    {
                        glm::mat4 l_InverseViewProjection = glm::inverse(camera.GetViewProjection());
                        m_SkyboxStage.Record(commandList, m_EnvironmentMap, l_InverseViewProjection, camera.GetPosition(), 1.0f);
                    }

                    DrawScene(commandList, scene, assetDatabase, camera);
                };
        }

        // Optional depth linearization pass for the editor's render-target viewer.
        if (m_DepthVisualize && m_DepthVis.IsValid())
        {
            m_RenderGraph.Import(m_DepthVis, ResourceState::Undefined, "DepthVis");

            RenderGraphPass& l_Pass = m_RenderGraph.AddPass("DepthVisualize");
            l_Pass.Reads.push_back(m_SceneDepth);

            RenderGraphColorTarget l_Color;
            l_Color.Target = m_DepthVis;
            l_Color.Clear = false;
            l_Pass.Colors.push_back(l_Color);

            l_Pass.Width = m_RenderWidth;
            l_Pass.Height = m_RenderHeight;
            l_Pass.ManageRendering = false;

            float l_Near = camera.GetNear();
            float l_Far = camera.GetFar();
            l_Pass.Execute = [this, l_Near, l_Far](CommandList& commandList)
                {
                    m_DepthVisualizeStage.Execute(commandList, m_SceneDepth, m_DepthVis, m_RenderWidth, m_RenderHeight, l_Near, l_Far);
                };
        }

        if (l_UseViewport)
        {
            // Post-process the HDR scene into the viewport color target.
            {
                RenderGraphPass& l_Pass = m_RenderGraph.AddPass("PostProcess");
                l_Pass.Reads.push_back(m_SceneColor);

                RenderGraphColorTarget l_Color;
                l_Color.Target = m_ViewportColor;
                l_Color.Clear = false;
                l_Pass.Colors.push_back(l_Color);

                l_Pass.Width = m_RenderWidth;
                l_Pass.Height = m_RenderHeight;
                l_Pass.ManageRendering = false;
                l_Pass.Execute = [this](CommandList& commandList)
                    {
                        m_PostProcess.Execute(commandList, m_SceneColor, m_ViewportColor, m_RenderWidth, m_RenderHeight, m_Exposure);
                    };
            }

            // Composite: clear the swapchain and draw the editor UI, which samples the viewport target.
            {
                RenderGraphPass& l_Pass = m_RenderGraph.AddPass("Composite");
                l_Pass.Reads.push_back(m_ViewportColor);
                if (m_DepthVisualize && m_DepthVis.IsValid())
                {
                    l_Pass.Reads.push_back(m_DepthVis);
                }

                RenderGraphColorTarget l_Color;
                l_Color.Target = l_Frame.BackBuffer;
                l_Color.Clear = true;
                l_Color.ClearColor[0] = 0.02f;
                l_Color.ClearColor[1] = 0.02f;
                l_Color.ClearColor[2] = 0.02f;
                l_Color.ClearColor[3] = 1.0f;
                l_Pass.Colors.push_back(l_Color);

                l_Pass.Width = l_SwapWidth;
                l_Pass.Height = l_SwapHeight;
                l_Pass.ManageRendering = true;
                l_Pass.Execute = [imgui](CommandList& commandList)
                    {
                        if (imgui != nullptr && imgui->IsInitialized())
                        {
                            imgui->EndFrame(commandList);
                        }
                    };
            }
        }
        else
        {
            // Post-process the HDR scene straight to the swapchain.
            {
                RenderGraphPass& l_Pass = m_RenderGraph.AddPass("PostProcess");
                l_Pass.Reads.push_back(m_SceneColor);

                RenderGraphColorTarget l_Color;
                l_Color.Target = l_Frame.BackBuffer;
                l_Color.Clear = false;
                l_Pass.Colors.push_back(l_Color);

                l_Pass.Width = l_SwapWidth;
                l_Pass.Height = l_SwapHeight;
                l_Pass.ManageRendering = false;

                TextureHandle l_BackBuffer = l_Frame.BackBuffer;
                l_Pass.Execute = [this, l_BackBuffer, l_SwapWidth, l_SwapHeight](CommandList& commandList)
                    {
                        m_PostProcess.Execute(commandList, m_SceneColor, l_BackBuffer, l_SwapWidth, l_SwapHeight, m_Exposure);
                    };
            }

            // Draw the editor UI directly over the swapchain.
            if (imgui != nullptr && imgui->IsInitialized())
            {
                RenderGraphPass& l_Pass = m_RenderGraph.AddPass("ImGui");
                if (m_DepthVisualize && m_DepthVis.IsValid())
                {
                    l_Pass.Reads.push_back(m_DepthVis);
                }

                RenderGraphColorTarget l_Color;
                l_Color.Target = l_Frame.BackBuffer;
                l_Color.Clear = false;
                l_Pass.Colors.push_back(l_Color);

                l_Pass.Width = l_SwapWidth;
                l_Pass.Height = l_SwapHeight;
                l_Pass.ManageRendering = true;
                l_Pass.Execute = [imgui](CommandList& commandList)
                    {
                        imgui->EndFrame(commandList);
                    };
            }
        }

        m_RenderGraph.SetPresent(l_Frame.BackBuffer);

        l_CommandList.Begin();

        if (!m_IblGenerated)
        {
            m_IBLProcessor.GenerateBrdfLut(l_CommandList, m_BrdfLut, k_BrdfLutSize);

            if (m_EnvironmentMap.IsValid())
            {
                m_IBLProcessor.GenerateIrradiance(l_CommandList, m_EnvironmentMap, m_IrradianceMap, k_IrradianceSize);
                m_IBLProcessor.GeneratePrefilter(l_CommandList, m_EnvironmentMap, m_PrefilteredMap, k_PrefilterSize, k_PrefilterMips);
            }
            else
            {
                m_IBLProcessor.ClearCube(l_CommandList, m_IrradianceMap, k_IrradianceSize, 1);
                m_IBLProcessor.ClearCube(l_CommandList, m_PrefilteredMap, k_PrefilterSize, k_PrefilterMips);
            }

            m_IblGenerated = true;
        }

        m_RenderGraph.Execute(l_CommandList);
        l_CommandList.End();

        m_Device.Submit(l_CommandList);
        m_Swapchain.Present();

        m_FrameIndex = (m_FrameIndex + 1) % static_cast<uint32_t>(m_CommandLists.size());
    }

    void Renderer::Resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            m_Minimized = true;

            return;
        }

        m_Minimized = false;

        m_Swapchain.Resize(width, height);

        if (!m_ViewportActive)
        {
            DestroySceneTargets();
            CreateSceneTargets(m_Swapchain.GetWidth(), m_Swapchain.GetHeight());
        }
    }
}