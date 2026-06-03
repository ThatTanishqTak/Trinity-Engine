#include <Trinity/Renderer/Frontend/Renderer.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Trinity/Renderer/Frontend/MeshVertex.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
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

    static MeshData BuildCubeMeshData()
    {
        MeshData l_Data;

        const glm::vec3 l_Normals[6] = { { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } };
        const glm::vec3 l_Tangents[6] = { { 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } };
        const glm::vec3 l_FaceVertices[6][4] =
        {
            { { -0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f } },
            { {  0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f,  0.5f } },
            { { -0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f, -0.5f } },
            { { -0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f } },
            { { -0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f,  0.5f }, { -0.5f, -0.5f,  0.5f } }
        };
        const glm::vec2 l_UVs[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

        for (int l_Face = 0; l_Face < 6; ++l_Face)
        {
            for (int l_Corner = 0; l_Corner < 4; ++l_Corner)
            {
                MeshVertex l_Vertex{};
                l_Vertex.Position = l_FaceVertices[l_Face][l_Corner];
                l_Vertex.Normal = l_Normals[l_Face];
                l_Vertex.Tangent = l_Tangents[l_Face];
                l_Vertex.UV = l_UVs[l_Corner];
                l_Data.Vertices.push_back(l_Vertex);
            }

            uint32_t l_Base = static_cast<uint32_t>(l_Face * 4);
            l_Data.Indices.push_back(l_Base + 0);
            l_Data.Indices.push_back(l_Base + 1);
            l_Data.Indices.push_back(l_Base + 2);
            l_Data.Indices.push_back(l_Base + 2);
            l_Data.Indices.push_back(l_Base + 3);
            l_Data.Indices.push_back(l_Base + 0);
        }

        Submesh l_Submesh;
        l_Submesh.IndexCount = static_cast<uint32_t>(l_Data.Indices.size());
        l_Submesh.Name = "Cube";
        l_Data.Submeshes.push_back(l_Submesh);

        MaterialSlot l_Slot;
        l_Slot.Name = "Default";
        l_Data.MaterialSlots.push_back(l_Slot);

        l_Data.Diagnostics.SourcePath = "<procedural cube>";
        l_Data.Diagnostics.SourceFormat = "procedural";

        return l_Data;
    }

    Renderer::Renderer(GraphicsDevice& device, Swapchain& swapchain, FileSystem& fileSystem) : m_Device(device), m_Swapchain(swapchain), m_FileSystem(fileSystem), m_TextureManager(device, fileSystem), m_Mesh(device)
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

        if (!CreateMesh())
        {
            return false;
        }

        if (!CreateTextureResources())
        {
            return false;
        }

        if (!CreateDepthTexture(m_Swapchain.GetWidth(), m_Swapchain.GetHeight()))
        {
            return false;
        }

        m_Camera.LookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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

        m_Mesh.Shutdown();

        DestroyDepthTexture();

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
        for (const ShaderVertexInput& a_Input : l_VertexResult.Reflection.VertexInputs)
        {
            TR_CORE_INFO("Renderer:   input '{}' @ location {}", a_Input.Name, a_Input.Location);
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
        l_PipelineDescription.ColorFormats = { m_Swapchain.GetFormat() };
        l_PipelineDescription.PushConstantSize = static_cast<uint32_t>(sizeof(glm::mat4));

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

    bool Renderer::CreateMesh()
    {
        const char* l_Candidates[] = { "Assets/Test.obj", "Assets/Test.fbx", "Assets/Test.gltf", "Assets/Test.glb", "Assets/Test.dae", "Assets/Test.3ds" };

        MeshData l_MeshData;
        bool l_Imported = false;
        for (const char* l_Candidate : l_Candidates)
        {
            std::filesystem::path l_Path = m_FileSystem.Resolve(BaseDirectory::Executable, l_Candidate);
            if (!std::filesystem::exists(l_Path))
            {
                continue;
            }

            std::optional<MeshData> l_Result = m_MeshImporter.Import(l_Path);
            if (l_Result.has_value())
            {
                l_MeshData = std::move(l_Result.value());
                l_Imported = true;

                break;
            }
        }

        if (!l_Imported)
        {
            TR_CORE_WARN("Renderer: no importable mesh in Assets, using procedural cube");
            l_MeshData = BuildCubeMeshData();
        }

        if (!m_Mesh.Upload(l_MeshData))
        {
            TR_CORE_ERROR("Renderer: mesh upload failed");
            return false;
        }

        return true;
    }

    bool Renderer::CreateTextureResources()
    {
        if (!m_TextureManager.Initialize())
        {
            return false;
        }

        m_Texture = m_TextureManager.Load("Assets/Test.png", true);

        return true;
    }

    bool Renderer::CreateDepthTexture(uint32_t width, uint32_t height)
    {
        TextureDescription l_Description;
        l_Description.Type = TextureType::Texture2D;
        l_Description.Format = Format::D32_SFLOAT;
        l_Description.Usage = TextureUsage::DepthStencil;
        l_Description.Width = width;
        l_Description.Height = height;
        l_Description.Depth = 1;
        l_Description.MipLevels = 1;
        l_Description.ArrayLayers = 1;
        l_Description.SampleCount = 1;
        l_Description.DebugName = "SceneDepth";

        m_DepthTexture = m_Device.CreateTexture(l_Description);
        if (!m_DepthTexture.IsValid())
        {
            TR_CORE_ERROR("Renderer: depth texture creation failed");
            return false;
        }

        return true;
    }

    void Renderer::DestroyDepthTexture()
    {
        if (m_DepthTexture.IsValid())
        {
            m_Device.DestroyTexture(m_DepthTexture);
            m_DepthTexture = TextureHandle{};
        }
    }

    void Renderer::RenderFrame()
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

        FrameInfo l_Frame;
        if (!m_Swapchain.AcquireNextImage(l_Frame))
        {
            return;
        }

        CommandList& l_CommandList = *m_CommandLists[m_FrameIndex];

        uint32_t l_Width = m_Swapchain.GetWidth();
        uint32_t l_Height = m_Swapchain.GetHeight();

        l_CommandList.Begin();
        l_CommandList.TransitionTexture(l_Frame.BackBuffer, ResourceState::Undefined, ResourceState::RenderTarget);
        l_CommandList.TransitionTexture(m_DepthTexture, ResourceState::Undefined, ResourceState::DepthStencil);

        RenderingAttachment l_ColorAttachment;
        l_ColorAttachment.Target = l_Frame.BackBuffer;
        l_ColorAttachment.Clear = true;
        l_ColorAttachment.ClearColor[0] = 0.05f;
        l_ColorAttachment.ClearColor[1] = 0.05f;
        l_ColorAttachment.ClearColor[2] = 0.05f;
        l_ColorAttachment.ClearColor[3] = 1.0f;

        DepthAttachment l_DepthAttachment;
        l_DepthAttachment.Target = m_DepthTexture;
        l_DepthAttachment.Clear = true;
        l_DepthAttachment.ClearDepth = 1.0f;

        RenderingInfo l_RenderingInfo;
        l_RenderingInfo.ColorAttachments = &l_ColorAttachment;
        l_RenderingInfo.ColorAttachmentCount = 1;
        l_RenderingInfo.Depth = &l_DepthAttachment;
        l_RenderingInfo.Width = l_Width;
        l_RenderingInfo.Height = l_Height;

        l_CommandList.BeginRendering(l_RenderingInfo);

        Viewport l_Viewport;
        l_Viewport.X = 0.0f;
        l_Viewport.Y = 0.0f;
        l_Viewport.Width = static_cast<float>(l_Width);
        l_Viewport.Height = static_cast<float>(l_Height);
        l_Viewport.MinDepth = 0.0f;
        l_Viewport.MaxDepth = 1.0f;
        l_CommandList.SetViewport(l_Viewport);

        Scissor l_Scissor;
        l_Scissor.X = 0;
        l_Scissor.Y = 0;
        l_Scissor.Width = l_Width;
        l_Scissor.Height = l_Height;
        l_CommandList.SetScissor(l_Scissor);

        l_CommandList.BindPipeline(m_Pipeline);
        l_CommandList.BindTexture(0, 0, m_Texture, m_TextureManager.DefaultSampler());
        l_CommandList.BindVertexBuffer(m_Mesh.GetVertexBuffer(), 0);
        l_CommandList.BindIndexBuffer(m_Mesh.GetIndexBuffer(), 0);

        float l_Aspect = l_Height > 0 ? static_cast<float>(l_Width) / static_cast<float>(l_Height) : 1.0f;
        m_Camera.SetPerspective(glm::radians(45.0f), l_Aspect, 0.1f, 100.0f);

        float l_Time = m_Timer.Elapsed();
        glm::mat4 l_Model = glm::rotate(glm::mat4(1.0f), l_Time * 0.8f, glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f)));
        glm::mat4 l_MVP = m_Camera.GetViewProjection() * l_Model;
        l_CommandList.PushConstants(ShaderStage::Vertex | ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(l_MVP)), &l_MVP);

        for (const Submesh& l_Submesh : m_Mesh.GetSubmeshes())
        {
            l_CommandList.DrawIndexed(l_Submesh.IndexCount, 1, l_Submesh.FirstIndex, static_cast<int32_t>(l_Submesh.BaseVertex), 0);
        }

        l_CommandList.EndRendering();
        l_CommandList.TransitionTexture(l_Frame.BackBuffer, ResourceState::RenderTarget, ResourceState::Present);
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

        DestroyDepthTexture();
        CreateDepthTexture(m_Swapchain.GetWidth(), m_Swapchain.GetHeight());
    }
}