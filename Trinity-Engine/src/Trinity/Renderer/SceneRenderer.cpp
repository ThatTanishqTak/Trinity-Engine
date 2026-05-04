#include "Trinity/Renderer/SceneRenderer.h"

#include "Trinity/Renderer/CommandBuffer.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/RenderGraph/RenderGraph.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Utilities/Log.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>

namespace Trinity
{
    namespace
    {
        constexpr uint32_t l_ShadowMapResolution = 2048;

        glm::mat4 ComputeLightViewProjection(const Camera& camera, const glm::vec3& sunDirection, uint32_t shadowMapResolution)
        {
            const glm::mat4 l_InverseViewProjection = glm::inverse(camera.GetViewProjectionMatrix());

            const glm::vec4 l_NdcCorners[8] =
            {
                { -1.0f, -1.0f, 0.0f, 1.0f },
                {  1.0f, -1.0f, 0.0f, 1.0f },
                { -1.0f,  1.0f, 0.0f, 1.0f },
                {  1.0f,  1.0f, 0.0f, 1.0f },
                { -1.0f, -1.0f, 1.0f, 1.0f },
                {  1.0f, -1.0f, 1.0f, 1.0f },
                { -1.0f,  1.0f, 1.0f, 1.0f },
                {  1.0f,  1.0f, 1.0f, 1.0f },
            };

            glm::vec3 l_FrustumCornersWorld[8];
            glm::vec3 l_FrustumCentre(0.0f);
            for (int i = 0; i < 8; ++i)
            {
                glm::vec4 l_Corner = l_InverseViewProjection * l_NdcCorners[i];
                l_FrustumCornersWorld[i] = glm::vec3(l_Corner) / l_Corner.w;
                l_FrustumCentre += l_FrustumCornersWorld[i];
            }
            l_FrustumCentre /= 8.0f;

            const glm::vec3 l_LightDirection = glm::normalize(sunDirection);
            const glm::vec3 l_Up = std::abs(l_LightDirection.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
            const glm::mat4 l_LightView = glm::lookAt(l_FrustumCentre - l_LightDirection, l_FrustumCentre, l_Up);

            glm::vec3 l_Min(std::numeric_limits<float>::max());
            glm::vec3 l_Max(std::numeric_limits<float>::lowest());
            for (int i = 0; i < 8; ++i)
            {
                const glm::vec3 l_LocalCorner = glm::vec3(l_LightView * glm::vec4(l_FrustumCornersWorld[i], 1.0f));
                l_Min = glm::min(l_Min, l_LocalCorner);
                l_Max = glm::max(l_Max, l_LocalCorner);
            }

            constexpr float l_ZExtension = 50.0f;
            l_Min.z -= l_ZExtension;

            const float l_WorldUnitsPerTexelX = (l_Max.x - l_Min.x) / static_cast<float>(shadowMapResolution);
            const float l_WorldUnitsPerTexelY = (l_Max.y - l_Min.y) / static_cast<float>(shadowMapResolution);

            if (l_WorldUnitsPerTexelX > 0.0f && l_WorldUnitsPerTexelY > 0.0f)
            {
                l_Min.x = std::floor(l_Min.x / l_WorldUnitsPerTexelX) * l_WorldUnitsPerTexelX;
                l_Max.x = std::floor(l_Max.x / l_WorldUnitsPerTexelX) * l_WorldUnitsPerTexelX;
                l_Min.y = std::floor(l_Min.y / l_WorldUnitsPerTexelY) * l_WorldUnitsPerTexelY;
                l_Max.y = std::floor(l_Max.y / l_WorldUnitsPerTexelY) * l_WorldUnitsPerTexelY;
            }

            glm::mat4 l_LightProjection = glm::ortho(l_Min.x, l_Max.x, l_Min.y, l_Max.y, l_Min.z, l_Max.z);
            l_LightProjection[1][1] *= -1.0f;

            return l_LightProjection * l_LightView;
        }
    }

    struct SceneRenderer::Implementation
    {
        SceneRendererStats Stats;
        bool SceneActive = false;
        std::unique_ptr<RenderGraph> Graph;

        std::shared_ptr<Pipeline> MeshPipeline;
        std::shared_ptr<Pipeline> ShadowPipeline;

        RenderGraphResourceHandle AlbedoHandle;
        RenderGraphResourceHandle NormalHandle;
        RenderGraphResourceHandle MetallicRoughnessAOHandle;
        RenderGraphResourceHandle DepthHandle;
        RenderGraphResourceHandle ShadowMapHandle;
    };

    SceneRenderer::SceneRenderer() = default;
    SceneRenderer::~SceneRenderer() = default;

    void SceneRenderer::Initialize(uint32_t width, uint32_t height)
    {
        TR_CORE_INFO("INITIALIZING SCENE RENDERER");

        m_Width = width;
        m_Height = height;
        m_Implementation = std::make_unique<Implementation>();

        m_Implementation->Graph = Renderer::GetAPI().CreateRenderGraph();
        if (!m_Implementation->Graph)
        {
            return;
        }

        m_Implementation->Graph->OnResize(width, height);

        RenderGraphTextureDescription l_AlbedoDescription{};
        l_AlbedoDescription.Format = TextureFormat::RGBA8;
        l_AlbedoDescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_AlbedoDescription.DebugName = "GeometryBuffer-Albedo";
        m_Implementation->AlbedoHandle = m_Implementation->Graph->DeclareTexture(l_AlbedoDescription);

        RenderGraphTextureDescription l_NormalDescription{};
        l_NormalDescription.Format = TextureFormat::RGBA16F;
        l_NormalDescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_NormalDescription.DebugName = "GeometryBuffer-Normal";
        m_Implementation->NormalHandle = m_Implementation->Graph->DeclareTexture(l_NormalDescription);

        RenderGraphTextureDescription l_MetallicRoughnessAODescription{};
        l_MetallicRoughnessAODescription.Format = TextureFormat::RGBA8;
        l_MetallicRoughnessAODescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_MetallicRoughnessAODescription.DebugName = "GeometryBuffer-MR-AO";
        m_Implementation->MetallicRoughnessAOHandle = m_Implementation->Graph->DeclareTexture(l_MetallicRoughnessAODescription);

        RenderGraphTextureDescription l_DepthDescription{};
        l_DepthDescription.Format = TextureFormat::Depth32F;
        l_DepthDescription.Usage = TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
        l_DepthDescription.DebugName = "GeometryBuffer-Depth";
        m_Implementation->DepthHandle = m_Implementation->Graph->DeclareTexture(l_DepthDescription);

        RenderGraphTextureDescription l_ShadowMapDescription{};
        l_ShadowMapDescription.Width = l_ShadowMapResolution;
        l_ShadowMapDescription.Height = l_ShadowMapResolution;
        l_ShadowMapDescription.Format = TextureFormat::Depth32F;
        l_ShadowMapDescription.Usage = TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
        l_ShadowMapDescription.DebugName = "ShadowMap-Depth";
        m_Implementation->ShadowMapHandle = m_Implementation->Graph->DeclareTexture(l_ShadowMapDescription);

        const auto a_AlbedoHandle = m_Implementation->AlbedoHandle;
        const auto a_NormalHandle = m_Implementation->NormalHandle;
        const auto a_MetallicRoughnessAOHandle = m_Implementation->MetallicRoughnessAOHandle;
        const auto a_DepthHandle = m_Implementation->DepthHandle;
        const auto a_ShadowMapHandle = m_Implementation->ShadowMapHandle;

        m_Implementation->Graph->AddPass("ShadowPass").Write(a_ShadowMapHandle).SetExecuteCallback([this, a_ShadowMapHandle](RenderGraphContext& context)
        {
            CommandBuffer& l_CommandBuffer = context.GetCommandBuffer();
            auto a_ShadowMap = context.GetTexture(a_ShadowMapHandle);
            if (!a_ShadowMap || !m_Implementation->ShadowPipeline)
            {
                return;
            }

            DepthAttachmentInfo l_DepthAttachment{};
            l_DepthAttachment.Image = a_ShadowMap.get();
            l_DepthAttachment.LoadOp = AttachmentLoadOp::Clear;
            l_DepthAttachment.StoreOp = AttachmentStoreOp::Store;
            l_DepthAttachment.ClearDepth = 1.0f;

            RenderingInfo l_RenderingInfo{};
            l_RenderingInfo.RenderAreaWidth = l_ShadowMapResolution;
            l_RenderingInfo.RenderAreaHeight = l_ShadowMapResolution;
            l_RenderingInfo.ColorAttachments = nullptr;
            l_RenderingInfo.ColorAttachmentCount = 0;
            l_RenderingInfo.DepthAttachment = &l_DepthAttachment;

            l_CommandBuffer.BeginRendering(l_RenderingInfo);

            Viewport l_Viewport{};
            l_Viewport.X = 0.0f;
            l_Viewport.Y = 0.0f;
            l_Viewport.Width = static_cast<float>(l_ShadowMapResolution);
            l_Viewport.Height = static_cast<float>(l_ShadowMapResolution);
            l_Viewport.MinDepth = 0.0f;
            l_Viewport.MaxDepth = 1.0f;
            l_CommandBuffer.SetViewport(l_Viewport);

            ScissorRect l_Scissor{};
            l_Scissor.X = 0;
            l_Scissor.Y = 0;
            l_Scissor.Width = l_ShadowMapResolution;
            l_Scissor.Height = l_ShadowMapResolution;
            l_CommandBuffer.SetScissor(l_Scissor);

            if (!m_DrawList.empty())
            {
                l_CommandBuffer.BindPipeline(*m_Implementation->ShadowPipeline);

                const glm::mat4 l_LightViewProjection = ComputeLightViewProjection(m_Camera, m_SceneData.SunDirection, l_ShadowMapResolution);

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

                    struct ShadowPushBlock
                    {
                        float Model[16];
                        float LightViewProjection[16];
                    } l_Push{};
                    std::memcpy(l_Push.Model, it_DrawCommand.Transform, sizeof(l_Push.Model));
                    std::memcpy(l_Push.LightViewProjection, glm::value_ptr(l_LightViewProjection), sizeof(l_Push.LightViewProjection));
                    l_CommandBuffer.PushConstants(ShaderStageFlags::Vertex, 0, sizeof(ShadowPushBlock), &l_Push);

                    l_CommandBuffer.DrawIndexed(it_DrawCommand.MeshRef->GetIndexCount(), 1, 0, 0, 0);
                }
            }

            l_CommandBuffer.EndRendering();
        });

        m_Implementation->Graph->AddPass("Geometry").Write(a_AlbedoHandle).Write(a_NormalHandle).Write(a_MetallicRoughnessAOHandle).Write(a_DepthHandle).Read(a_ShadowMapHandle).SetExecuteCallback([this, a_AlbedoHandle, a_NormalHandle, a_MetallicRoughnessAOHandle, a_DepthHandle](RenderGraphContext& context)
        {
            CommandBuffer& l_CommandBuffer = context.GetCommandBuffer();

            auto a_Albedo = context.GetTexture(a_AlbedoHandle);
            auto a_Normal = context.GetTexture(a_NormalHandle);
            auto a_MetallicRoughnessAO = context.GetTexture(a_MetallicRoughnessAOHandle);
            auto a_Depth = context.GetTexture(a_DepthHandle);

            if (!a_Albedo || !a_Normal || !a_MetallicRoughnessAO || !a_Depth)
            {
                return;
            }

            ColorAttachmentInfo l_ColorAttachments[3]{};
            l_ColorAttachments[0].Image = a_Albedo.get();
            l_ColorAttachments[0].LoadOp = AttachmentLoadOp::Clear;
            l_ColorAttachments[0].StoreOp = AttachmentStoreOp::Store;
            l_ColorAttachments[0].ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

            l_ColorAttachments[1].Image = a_Normal.get();
            l_ColorAttachments[1].LoadOp = AttachmentLoadOp::Clear;
            l_ColorAttachments[1].StoreOp = AttachmentStoreOp::Store;
            l_ColorAttachments[1].ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

            l_ColorAttachments[2].Image = a_MetallicRoughnessAO.get();
            l_ColorAttachments[2].LoadOp = AttachmentLoadOp::Clear;
            l_ColorAttachments[2].StoreOp = AttachmentStoreOp::Store;
            l_ColorAttachments[2].ClearColor = { 0.0f, 0.5f, 1.0f, 0.0f };

            DepthAttachmentInfo l_DepthAttachment{};
            l_DepthAttachment.Image = a_Depth.get();
            l_DepthAttachment.LoadOp = AttachmentLoadOp::Clear;
            l_DepthAttachment.StoreOp = AttachmentStoreOp::Store;
            l_DepthAttachment.ClearDepth = 1.0f;

            RenderingInfo l_RenderingInfo{};
            l_RenderingInfo.RenderAreaWidth = m_Width;
            l_RenderingInfo.RenderAreaHeight = m_Height;
            l_RenderingInfo.ColorAttachments = l_ColorAttachments;
            l_RenderingInfo.ColorAttachmentCount = 3;
            l_RenderingInfo.DepthAttachment = &l_DepthAttachment;

            l_CommandBuffer.BeginRendering(l_RenderingInfo);

            Viewport l_Viewport{};
            l_Viewport.X = 0.0f;
            l_Viewport.Y = 0.0f;
            l_Viewport.Width = static_cast<float>(m_Width);
            l_Viewport.Height = static_cast<float>(m_Height);
            l_Viewport.MinDepth = 0.0f;
            l_Viewport.MaxDepth = 1.0f;
            l_CommandBuffer.SetViewport(l_Viewport);

            ScissorRect l_Scissor{};
            l_Scissor.X = 0;
            l_Scissor.Y = 0;
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
                        float Model[16];
                        float ViewProjection[16];
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

        m_Implementation->Graph->AddPass("Output").Read(a_AlbedoHandle);

        ShaderSpecification l_ShadowShaderSpecification{};
        l_ShadowShaderSpecification.Modules.push_back({ ShaderStage::Vertex, "shadow_pass.vert.spv" });
        l_ShadowShaderSpecification.Modules.push_back({ ShaderStage::Fragment, "shadow_pass.frag.spv" });
        l_ShadowShaderSpecification.DebugName = "ShadowShader";
        auto a_ShadowShader = Renderer::GetAPI().CreateShader(l_ShadowShaderSpecification);

        PipelineSpecification l_ShadowPipelineSpecification{};
        l_ShadowPipelineSpecification.PipelineShader = a_ShadowShader;
        l_ShadowPipelineSpecification.VertexAttributes =
        {
            { 0, 0, VertexAttributeFormat::Float3, 0 },
            { 1, 0, VertexAttributeFormat::Float3, 12 },
            { 2, 0, VertexAttributeFormat::Float2, 24 },
        };
        l_ShadowPipelineSpecification.VertexStride = sizeof(Geometry::Vertex);
        l_ShadowPipelineSpecification.PushConstants = { { ShaderStage::Vertex, 0, 128 } };
        l_ShadowPipelineSpecification.ColorAttachmentFormats = {};
        l_ShadowPipelineSpecification.DepthAttachmentFormat = TextureFormat::Depth32F;
        l_ShadowPipelineSpecification.DepthTest = true;
        l_ShadowPipelineSpecification.DepthWrite = true;
        l_ShadowPipelineSpecification.DepthOp = DepthCompareOp::Less;
        l_ShadowPipelineSpecification.CullingMode = CullMode::Front;
        l_ShadowPipelineSpecification.DepthBias = true;
        l_ShadowPipelineSpecification.DepthBiasConstantFactor = 1.25f;
        l_ShadowPipelineSpecification.DepthBiasSlopeFactor = 1.75f;
        l_ShadowPipelineSpecification.DebugName = "ShadowPipeline";
        m_Implementation->ShadowPipeline = Renderer::GetAPI().CreatePipeline(l_ShadowPipelineSpecification);

        ShaderSpecification l_GeometryShaderSpecification{};
        l_GeometryShaderSpecification.Modules.push_back({ ShaderStage::Vertex, "geometry_pass.vert.spv" });
        l_GeometryShaderSpecification.Modules.push_back({ ShaderStage::Fragment, "geometry_pass.frag.spv" });
        l_GeometryShaderSpecification.DebugName = "MeshShader";
        auto a_Shader = Renderer::GetAPI().CreateShader(l_GeometryShaderSpecification);

        PipelineSpecification l_GeometryPipelineSpecification{};
        l_GeometryPipelineSpecification.PipelineShader = a_Shader;
        l_GeometryPipelineSpecification.VertexAttributes =
        {
            { 0, 0, VertexAttributeFormat::Float3, 0 },
            { 1, 0, VertexAttributeFormat::Float3, 12 },
            { 2, 0, VertexAttributeFormat::Float2, 24 },
        };
        l_GeometryPipelineSpecification.VertexStride = sizeof(Geometry::Vertex);
        l_GeometryPipelineSpecification.PushConstants = { { ShaderStage::Vertex, 0, 128 } };
        l_GeometryPipelineSpecification.ColorAttachmentFormats = { TextureFormat::RGBA8, TextureFormat::RGBA16F, TextureFormat::RGBA8 };
        l_GeometryPipelineSpecification.DepthAttachmentFormat = TextureFormat::Depth32F;
        l_GeometryPipelineSpecification.DepthTest = true;
        l_GeometryPipelineSpecification.DepthWrite = true;
        l_GeometryPipelineSpecification.DepthOp = DepthCompareOp::Less;
        l_GeometryPipelineSpecification.CullingMode = CullMode::None;
        l_GeometryPipelineSpecification.DebugName = "MeshPipeline";
        m_Implementation->MeshPipeline = Renderer::GetAPI().CreatePipeline(l_GeometryPipelineSpecification);

        TR_CORE_INFO("SCENE RENDERER INITIALIZED");
    }

    void SceneRenderer::Shutdown()
    {
        TR_CORE_INFO("SHUTTING DOWN SCENE RENDERER");

        m_Implementation.reset();

        TR_CORE_INFO("SCENE RENDERER SHUTDOWN COMPLETE");
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

        return m_Implementation->Graph->GetTexture(m_Implementation->AlbedoHandle);
    }

    const SceneRendererStats& SceneRenderer::GetStats() const
    {
        return m_Implementation->Stats;
    }
}