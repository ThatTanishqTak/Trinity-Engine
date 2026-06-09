#include <Trinity/Renderer/Frontend/Renderer.h>
#include <Trinity/ImGui/ImGuiLayer.h>
#include <Trinity/ImGui/IImGuiRenderBackend.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Core/Log.h>

#include <Trinity/Renderer/Meshes/Mesh.h>
#include <Trinity/Renderer/Materials/Material.h>
#include <Trinity/Renderer/Materials/MaterialParameter.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Assets/AssetDatabase.h>

namespace Trinity
{
    struct MeshPushConstants
    {
        glm::mat4 MVP;
        glm::vec4 BaseColorFactor;
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

        m_PostProcess.Shutdown();

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

        ResourceBinding l_TextureBinding;
        l_TextureBinding.Set = 0;
        l_TextureBinding.Binding = 0;
        l_TextureBinding.Type = ResourceBindingType::CombinedImageSampler;
        l_TextureBinding.Stages = ShaderStage::Fragment;
        l_PipelineDescription.Bindings = { l_TextureBinding };
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
        l_DepthDescription.Usage = TextureUsage::DepthStencil;
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

    void Renderer::DrawScene(CommandList& commandList, Scene& scene, AssetDatabase& assetDatabase, const Camera& camera)
    {
        commandList.BindPipeline(m_Pipeline);

        glm::mat4 l_ViewProjection = camera.GetViewProjection();

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

            glm::mat4 l_ModelViewProjection = l_ViewProjection * scene.GetWorldMatrix(l_Entity);

            commandList.BindVertexBuffer(l_Mesh.GetVertexBuffer(), 0);
            commandList.BindIndexBuffer(l_Mesh.GetIndexBuffer(), 0);

            for (const Submesh& l_Submesh : l_Mesh.GetSubmeshes())
            {
                MeshPushConstants l_PushConstants;
                l_PushConstants.MVP = l_ModelViewProjection;
                l_PushConstants.BaseColorFactor = glm::vec4(1.0f);

                TextureHandle l_BaseColor = m_TextureManager.White();

                UUID l_MaterialAsset = l_Submesh.MaterialIndex < l_MeshRenderer.Materials.size() ? l_MeshRenderer.Materials[l_Submesh.MaterialIndex] : UUID(0);
                if (static_cast<uint64_t>(l_MaterialAsset) != 0)
                {
                    std::shared_ptr<Material> l_Material = assetDatabase.ResolveMaterial(l_MaterialAsset);
                    if (const MaterialParameter* l_Factor = l_Material->FindParameter(Material::BaseColorFactor))
                    {
                        l_PushConstants.BaseColorFactor = l_Factor->AsVec4();
                    }

                    if (const MaterialParameter* l_Texture = l_Material->FindParameter(Material::BaseColorTexture))
                    {
                        l_BaseColor = assetDatabase.ResolveTexture(l_Texture->AsTexture());
                    }
                }
                else if (l_Submesh.MaterialIndex < l_Slots.size())
                {
                    l_PushConstants.BaseColorFactor = l_Slots[l_Submesh.MaterialIndex].BaseColorFactor;
                }

                commandList.BindTexture(0, 0, l_BaseColor, m_TextureManager.DefaultSampler());
                commandList.PushConstants(ShaderStage::Vertex | ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(l_PushConstants)), &l_PushConstants);

                commandList.DrawIndexed(l_Submesh.IndexCount, 1, l_Submesh.FirstIndex, static_cast<int32_t>(l_Submesh.BaseVertex), 0);
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

        l_CommandList.Begin();

        l_CommandList.TransitionTexture(m_SceneColor, ResourceState::Undefined, ResourceState::RenderTarget);
        l_CommandList.TransitionTexture(m_SceneDepth, ResourceState::Undefined, ResourceState::DepthStencil);

        RenderingAttachment l_SceneColorAttachment;
        l_SceneColorAttachment.Target = m_SceneColor;
        l_SceneColorAttachment.Clear = true;
        l_SceneColorAttachment.ClearColor[0] = 0.0f;
        l_SceneColorAttachment.ClearColor[1] = 0.0f;
        l_SceneColorAttachment.ClearColor[2] = 0.0f;
        l_SceneColorAttachment.ClearColor[3] = 1.0f;

        DepthAttachment l_SceneDepthAttachment;
        l_SceneDepthAttachment.Target = m_SceneDepth;
        l_SceneDepthAttachment.Clear = true;
        l_SceneDepthAttachment.ClearDepth = 1.0f;

        RenderingInfo l_SceneInfo;
        l_SceneInfo.ColorAttachments = &l_SceneColorAttachment;
        l_SceneInfo.ColorAttachmentCount = 1;
        l_SceneInfo.Depth = &l_SceneDepthAttachment;
        l_SceneInfo.Width = m_RenderWidth;
        l_SceneInfo.Height = m_RenderHeight;

        l_CommandList.BeginRendering(l_SceneInfo);

        Viewport l_SceneViewport;
        l_SceneViewport.X = 0.0f;
        l_SceneViewport.Y = 0.0f;
        l_SceneViewport.Width = static_cast<float>(m_RenderWidth);
        l_SceneViewport.Height = static_cast<float>(m_RenderHeight);
        l_SceneViewport.MinDepth = 0.0f;
        l_SceneViewport.MaxDepth = 1.0f;
        l_CommandList.SetViewport(l_SceneViewport);

        Scissor l_SceneScissor;
        l_SceneScissor.X = 0;
        l_SceneScissor.Y = 0;
        l_SceneScissor.Width = m_RenderWidth;
        l_SceneScissor.Height = m_RenderHeight;
        l_CommandList.SetScissor(l_SceneScissor);

        DrawScene(l_CommandList, scene, assetDatabase, camera);

        l_CommandList.EndRendering();

        l_CommandList.TransitionTexture(m_SceneColor, ResourceState::RenderTarget, ResourceState::ShaderResource);

        if (l_UseViewport)
        {
            l_CommandList.TransitionTexture(m_ViewportColor, ResourceState::Undefined, ResourceState::RenderTarget);

            m_PostProcess.Execute(l_CommandList, m_SceneColor, m_ViewportColor, m_RenderWidth, m_RenderHeight, m_Exposure);

            l_CommandList.TransitionTexture(m_ViewportColor, ResourceState::RenderTarget, ResourceState::ShaderResource);
            l_CommandList.TransitionTexture(l_Frame.BackBuffer, ResourceState::Undefined, ResourceState::RenderTarget);

            RenderingAttachment l_BackColor;
            l_BackColor.Target = l_Frame.BackBuffer;
            l_BackColor.Clear = true;
            l_BackColor.ClearColor[0] = 0.02f;
            l_BackColor.ClearColor[1] = 0.02f;
            l_BackColor.ClearColor[2] = 0.02f;
            l_BackColor.ClearColor[3] = 1.0f;

            RenderingInfo l_BackInfo;
            l_BackInfo.ColorAttachments = &l_BackColor;
            l_BackInfo.ColorAttachmentCount = 1;
            l_BackInfo.Depth = nullptr;
            l_BackInfo.Width = l_SwapWidth;
            l_BackInfo.Height = l_SwapHeight;

            l_CommandList.BeginRendering(l_BackInfo);

            if (imgui != nullptr && imgui->IsInitialized())
            {
                imgui->EndFrame(l_CommandList);
            }

            l_CommandList.EndRendering();

            l_CommandList.TransitionTexture(l_Frame.BackBuffer, ResourceState::RenderTarget, ResourceState::Present);
        }
        else
        {
            l_CommandList.TransitionTexture(l_Frame.BackBuffer, ResourceState::Undefined, ResourceState::RenderTarget);

            m_PostProcess.Execute(l_CommandList, m_SceneColor, l_Frame.BackBuffer, l_SwapWidth, l_SwapHeight, m_Exposure);

            if (imgui != nullptr && imgui->IsInitialized())
            {
                l_CommandList.TransitionTexture(l_Frame.BackBuffer, ResourceState::RenderTarget, ResourceState::RenderTarget);

                RenderingAttachment l_ImGuiColor;
                l_ImGuiColor.Target = l_Frame.BackBuffer;
                l_ImGuiColor.Clear = false;

                RenderingInfo l_ImGuiInfo;
                l_ImGuiInfo.ColorAttachments = &l_ImGuiColor;
                l_ImGuiInfo.ColorAttachmentCount = 1;
                l_ImGuiInfo.Depth = nullptr;
                l_ImGuiInfo.Width = l_SwapWidth;
                l_ImGuiInfo.Height = l_SwapHeight;

                l_CommandList.BeginRendering(l_ImGuiInfo);
                imgui->EndFrame(l_CommandList);
                l_CommandList.EndRendering();
            }

            l_CommandList.TransitionTexture(l_Frame.BackBuffer, ResourceState::RenderTarget, ResourceState::Present);
        }

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