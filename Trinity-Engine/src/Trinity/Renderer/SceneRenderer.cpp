#include "Trinity/Renderer/SceneRenderer.h"

#include "Trinity/Renderer/CommandBuffer.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/RenderGraph/RenderGraph.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Utilities/Log.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#include <memory>

namespace Trinity
{
    struct SceneRenderer::Implementation
    {
        SceneRendererStats Stats;
        bool SceneActive = false;
        std::unique_ptr<RenderGraph> Graph;
        std::shared_ptr<Pipeline> MeshPipeline;
        RenderGraphResourceHandle ColorHandle;
        RenderGraphResourceHandle NormalHandle;
        RenderGraphResourceHandle DepthHandle;
    };

    SceneRenderer::SceneRenderer() = default;
    SceneRenderer::~SceneRenderer() = default;

    void SceneRenderer::Initialize(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;
        m_Implementation = std::make_unique<Implementation>();

        m_Implementation->Graph = Renderer::GetAPI().CreateRenderGraph();
        if (!m_Implementation->Graph)
        {
            TR_CORE_ERROR("[SCENE RENDERER]: Backend produced a null render graph");
            return;
        }

        m_Implementation->Graph->OnResize(width, height);

        RenderGraphTextureDescription l_ColorDescription{};
        l_ColorDescription.Format = TextureFormat::RGBA16F;
        l_ColorDescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_ColorDescription.DebugName = "GeometryBuffer-Color";
        m_Implementation->ColorHandle = m_Implementation->Graph->DeclareTexture(l_ColorDescription);

        RenderGraphTextureDescription l_NormalDescription{};
        l_NormalDescription.Format = TextureFormat::RGBA16F;
        l_NormalDescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_NormalDescription.DebugName = "GeometryBuffer-Normal";
        m_Implementation->NormalHandle = m_Implementation->Graph->DeclareTexture(l_NormalDescription);

        RenderGraphTextureDescription l_DepthDescription{};
        l_DepthDescription.Format = TextureFormat::Depth32F;
        l_DepthDescription.Usage = TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
        l_DepthDescription.DebugName = "GeometryBuffer-Depth";
        m_Implementation->DepthHandle = m_Implementation->Graph->DeclareTexture(l_DepthDescription);

        const auto a_ColorHandle = m_Implementation->ColorHandle;
        const auto a_NormalHandle = m_Implementation->NormalHandle;
        const auto a_DepthHandle = m_Implementation->DepthHandle;

        m_Implementation->Graph->AddPass("Geometry").Write(a_ColorHandle).Write(a_NormalHandle).Write(a_DepthHandle).SetExecuteCallback([this, a_ColorHandle, a_NormalHandle, a_DepthHandle](RenderGraphContext& context)
        {
            CommandBuffer& l_CommandBuffer = context.GetCommandBuffer();

            auto a_Color = context.GetTexture(a_ColorHandle);
            auto a_Normal = context.GetTexture(a_NormalHandle);
            auto a_Depth = context.GetTexture(a_DepthHandle);

            if (!a_Color || !a_Normal || !a_Depth)
            {
                return;
            }

            ColorAttachmentInfo l_ColorAttachments[2]{};
            l_ColorAttachments[0].Image = a_Color.get();
            l_ColorAttachments[0].LoadOp = AttachmentLoadOp::Clear;
            l_ColorAttachments[0].StoreOp = AttachmentStoreOp::Store;
            l_ColorAttachments[0].ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

            l_ColorAttachments[1].Image = a_Normal.get();
            l_ColorAttachments[1].LoadOp = AttachmentLoadOp::Clear;
            l_ColorAttachments[1].StoreOp = AttachmentStoreOp::Store;
            l_ColorAttachments[1].ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

            DepthAttachmentInfo l_DepthAttachment{};
            l_DepthAttachment.Image = a_Depth.get();
            l_DepthAttachment.LoadOp = AttachmentLoadOp::Clear;
            l_DepthAttachment.StoreOp = AttachmentStoreOp::Store;
            l_DepthAttachment.ClearDepth = 1.0f;

            RenderingInfo l_RenderingInfo{};
            l_RenderingInfo.RenderAreaWidth = m_Width;
            l_RenderingInfo.RenderAreaHeight = m_Height;
            l_RenderingInfo.ColorAttachments = l_ColorAttachments;
            l_RenderingInfo.ColorAttachmentCount = 2;
            l_RenderingInfo.DepthAttachment = &l_DepthAttachment;

            l_CommandBuffer.BeginRendering(l_RenderingInfo);

            Viewport l_Viewport{};
            l_Viewport.Width = static_cast<float>(m_Width);
            l_Viewport.Height = static_cast<float>(m_Height);
            l_Viewport.MinDepth = 0.0f;
            l_Viewport.MaxDepth = 1.0f;
            l_CommandBuffer.SetViewport(l_Viewport);

            ScissorRect l_Scissor{};
            l_Scissor.Width = m_Width;
            l_Scissor.Height = m_Height;
            l_CommandBuffer.SetScissor(l_Scissor);

            if (!m_DrawList.empty() && m_Implementation->MeshPipeline)
            {
                l_CommandBuffer.BindPipeline(*m_Implementation->MeshPipeline);

                const glm::mat4 l_ViewProjection = m_Camera.GetViewProjectionMatrix();

                for (const auto& it_DrawCommand : m_DrawList)
                {
                    if (!it_DrawCommand.MeshRef)
                    {
                        continue;
                    }

                    auto a_VertexBuffer = it_DrawCommand.MeshRef->GetVertexBuffer();
                    auto a_IndexBuffer = it_DrawCommand.MeshRef->GetIndexBuffer();
                    if (!a_VertexBuffer || !a_IndexBuffer)
                    {
                        continue;
                    }

                    l_CommandBuffer.BindVertexBuffer(0, *a_VertexBuffer);
                    l_CommandBuffer.BindIndexBuffer(*a_IndexBuffer, IndexType::UInt32);

                    struct PushBlock
                    {
                        float Model[16]; float ViewProjection[16];
                    } l_Push{};
                    std::memcpy(l_Push.Model, it_DrawCommand.Transform, sizeof(l_Push.Model));
                    std::memcpy(l_Push.ViewProjection, glm::value_ptr(l_ViewProjection), sizeof(l_Push.ViewProjection));
                    l_CommandBuffer.PushConstants(ShaderStageFlags::Vertex, 0, sizeof(PushBlock), &l_Push);

                    l_CommandBuffer.DrawIndexed(it_DrawCommand.MeshRef->GetIndexCount(), 1, 0, 0, 0);

                    m_Implementation->Stats.DrawCalls++;
                    m_Implementation->Stats.IndexCount += it_DrawCommand.MeshRef->GetIndexCount();
                    m_Implementation->Stats.VertexCount += it_DrawCommand.MeshRef->GetVertexCount();
                }
            }

            l_CommandBuffer.EndRendering();
        });

        m_Implementation->Graph->AddPass("Output").Read(a_ColorHandle);

        ShaderSpecification l_ShaderSpecification{};
        l_ShaderSpecification.Modules.push_back({ ShaderStage::Vertex,   "geometry_pass.vert.spv" });
        l_ShaderSpecification.Modules.push_back({ ShaderStage::Fragment, "geometry_pass.frag.spv" });
        l_ShaderSpecification.DebugName = "MeshShader";
        auto a_Shader = Renderer::GetAPI().CreateShader(l_ShaderSpecification);

        PipelineSpecification l_PipelineSpecification{};
        l_PipelineSpecification.PipelineShader = a_Shader;
        l_PipelineSpecification.VertexAttributes =
        {
            { 0, 0, VertexAttributeFormat::Float3, 0 },
            { 1, 0, VertexAttributeFormat::Float3, 12 },
            { 2, 0, VertexAttributeFormat::Float2, 24 },
        };
        l_PipelineSpecification.VertexStride = sizeof(Geometry::Vertex);
        l_PipelineSpecification.PushConstants = { { ShaderStage::Vertex, 0, 128 } };
        l_PipelineSpecification.ColorAttachmentFormats = { TextureFormat::RGBA16F, TextureFormat::RGBA16F };
        l_PipelineSpecification.DepthAttachmentFormat = TextureFormat::Depth32F;
        l_PipelineSpecification.DepthTest = true;
        l_PipelineSpecification.DepthWrite = true;
        l_PipelineSpecification.DepthOp = DepthCompareOp::Less;
        l_PipelineSpecification.CullingMode = CullMode::Back;
        l_PipelineSpecification.DebugName = "MeshPipeline";
        m_Implementation->MeshPipeline = Renderer::GetAPI().CreatePipeline(l_PipelineSpecification);
    }

    void SceneRenderer::Shutdown()
    {
        m_Implementation.reset();
    }

    void SceneRenderer::BeginScene(const Camera& camera, const SceneRenderData& sceneData)
    {
        m_Camera = camera;
        m_SceneData = sceneData;
        m_DrawList.clear();
        m_Implementation->Stats = {};
        m_Implementation->SceneActive = true;
    }

    void SceneRenderer::SubmitMesh(const MeshDrawCommand& command)
    {
        m_DrawList.push_back(command);
    }

    void SceneRenderer::EndScene()
    {
        m_Implementation->SceneActive = false;
    }

    void SceneRenderer::Render()
    {
        if (!m_Implementation || !m_Implementation->Graph || !m_Implementation->MeshPipeline)
        {
            return;
        }

        m_Implementation->Graph->Execute();
    }

    void SceneRenderer::OnResize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        if (m_Width == width && m_Height == height)
        {
            return;
        }

        Renderer::WaitIdle();

        m_Width = width;
        m_Height = height;

        if (m_Implementation && m_Implementation->Graph)
        {
            m_Implementation->Graph->OnResize(width, height);
        }
    }

    std::shared_ptr<Texture> SceneRenderer::GetFinalOutput() const
    {
        if (!m_Implementation || !m_Implementation->Graph)
        {
            return nullptr;
        }

        return m_Implementation->Graph->GetTexture(m_Implementation->ColorHandle);
    }

    const SceneRendererStats& SceneRenderer::GetStats() const
    {
        return m_Implementation->Stats;
    }
}