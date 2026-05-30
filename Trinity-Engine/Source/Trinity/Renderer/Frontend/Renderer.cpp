#include <Trinity/Renderer/Frontend/Renderer.h>

#include <glm/glm.hpp>

#include <Trinity/Renderer/Frontend/Vertex.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Core/FileManagement.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    Renderer::Renderer(GraphicsDevice& device, Swapchain& swapchain, FileSystem& fileSystem) : m_Device(device), m_Swapchain(swapchain), m_FileSystem(fileSystem)
    {

    }

    Renderer::~Renderer()
    {
        Shutdown();
    }

    bool Renderer::Initialize()
    {
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

        if (!CreateGeometry())
        {
            return false;
        }

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

        if (m_VertexBuffer.IsValid())
        {
            m_Device.DestroyBuffer(m_VertexBuffer);
            m_VertexBuffer = BufferHandle{};
        }
    }

    bool Renderer::CreatePipeline()
    {
        auto a_LoadShader = [&](const char* name) -> std::vector<uint8_t>
        {
            std::filesystem::path l_Path = m_FileSystem.Resolve(BaseDirectory::Executable, std::filesystem::path("Shaders") / name);
            std::optional<std::vector<uint8_t>> l_Bytes = FileManagement::ReadBinary(l_Path);
            if (!l_Bytes)
            {
                TR_CORE_ERROR("Renderer: failed to load shader {}", name);
                return {};
            }

            return *l_Bytes;
        };

        std::vector<uint8_t> l_VertexBytes = a_LoadShader("Triangle.vert.spv");
        std::vector<uint8_t> l_FragmentBytes = a_LoadShader("Triangle.frag.spv");
        if (l_VertexBytes.empty() || l_FragmentBytes.empty())
        {
            return false;
        }

        ShaderDescription l_VertexShaderDescription;
        l_VertexShaderDescription.Stage = ShaderStage::Vertex;
        l_VertexShaderDescription.Bytecode = l_VertexBytes;
        l_VertexShaderDescription.DebugName = "Triangle.vert";
        m_VertexShader = m_Device.CreateShader(l_VertexShaderDescription);

        ShaderDescription l_FragmentShaderDescription;
        l_FragmentShaderDescription.Stage = ShaderStage::Fragment;
        l_FragmentShaderDescription.Bytecode = l_FragmentBytes;
        l_FragmentShaderDescription.DebugName = "Triangle.frag";
        m_FragmentShader = m_Device.CreateShader(l_FragmentShaderDescription);

        if (!m_VertexShader.IsValid() || !m_FragmentShader.IsValid())
        {
            TR_CORE_ERROR("Renderer: shader module creation failed");
            return false;
        }

        PipelineDescription l_PipelineDescription;
        l_PipelineDescription.VertexShader = m_VertexShader;
        l_PipelineDescription.FragmentShader = m_FragmentShader;
        l_PipelineDescription.Vertex = Vertex::GetLayout();
        l_PipelineDescription.Topology = PrimitiveTopology::TriangleList;
        l_PipelineDescription.Rasterizer.Cull = CullMode::None;
        l_PipelineDescription.DepthStencil.DepthTest = false;
        l_PipelineDescription.DepthStencil.DepthWrite = false;
        l_PipelineDescription.DepthFormat = Format::Unknown;
        l_PipelineDescription.ColorFormats = { m_Swapchain.GetFormat() };
        l_PipelineDescription.PushConstantSize = static_cast<uint32_t>(sizeof(glm::mat4));
        l_PipelineDescription.DebugName = "Triangle";

        m_Pipeline = m_Device.CreatePipeline(l_PipelineDescription);
        if (!m_Pipeline.IsValid())
        {
            TR_CORE_ERROR("Renderer: pipeline creation failed");
            return false;
        }

        return true;
    }

    bool Renderer::CreateGeometry()
    {
        const Vertex l_Vertices[] =
        {
            { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { { 0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
        };

        m_VertexCount = static_cast<uint32_t>(sizeof(l_Vertices) / sizeof(Vertex));

        BufferDescription l_BufferDescription;
        l_BufferDescription.Size = sizeof(l_Vertices);
        l_BufferDescription.Usage = BufferUsage::Vertex;
        l_BufferDescription.Memory = MemoryUsage::GpuOnly;
        l_BufferDescription.InitialData = l_Vertices;
        l_BufferDescription.DebugName = "TriangleVertices";

        m_VertexBuffer = m_Device.CreateBuffer(l_BufferDescription);
        if (!m_VertexBuffer.IsValid())
        {
            TR_CORE_ERROR("Renderer: vertex buffer creation failed");
            return false;
        }

        return true;
    }

    void Renderer::RenderFrame()
    {
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

        RenderingAttachment l_ColorAttachment;
        l_ColorAttachment.Target = l_Frame.BackBuffer;
        l_ColorAttachment.Clear = true;
        l_ColorAttachment.ClearColor[0] = 0.05f;
        l_ColorAttachment.ClearColor[1] = 0.05f;
        l_ColorAttachment.ClearColor[2] = 0.05f;
        l_ColorAttachment.ClearColor[3] = 1.0f;

        RenderingInfo l_RenderingInfo;
        l_RenderingInfo.ColorAttachments = &l_ColorAttachment;
        l_RenderingInfo.ColorAttachmentCount = 1;
        l_RenderingInfo.Depth = nullptr;
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
        l_CommandList.BindVertexBuffer(m_VertexBuffer, 0);

        glm::mat4 l_MVP(1.0f);
        l_CommandList.PushConstants(ShaderStage::Vertex | ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(l_MVP)), &l_MVP);

        l_CommandList.Draw(m_VertexCount, 1, 0, 0);

        l_CommandList.EndRendering();
        l_CommandList.TransitionTexture(l_Frame.BackBuffer, ResourceState::RenderTarget, ResourceState::Present);
        l_CommandList.End();

        m_Device.Submit(l_CommandList);
        m_Swapchain.Present();

        m_FrameIndex = (m_FrameIndex + 1) % static_cast<uint32_t>(m_CommandLists.size());
    }

    void Renderer::Resize(uint32_t width, uint32_t height)
    {
        m_Swapchain.Resize(width, height);
    }
}