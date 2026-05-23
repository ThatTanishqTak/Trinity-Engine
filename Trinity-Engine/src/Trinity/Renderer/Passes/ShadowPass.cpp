#include "Trinity/Renderer/Passes/ShadowPass.h"

#include "Trinity/Renderer/CommandList.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/RenderGraph/RenderGraph.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Renderer/Resources/Texture.h"
#include "Trinity/Utilities/Log.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cstddef>

namespace Trinity
{
    namespace
    {
        struct ShadowPushBlock
        {
            glm::mat4 Model;
            glm::mat4 LightViewProjection;
        };
    }

    void ShadowPass::Initialize()
    {
        ShaderSpecification l_ShaderSpecification{};
        l_ShaderSpecification.Modules.push_back({ ShaderStage::Vertex, "shadow_pass.vert.spv" });
        l_ShaderSpecification.Modules.push_back({ ShaderStage::Fragment, "shadow_pass.frag.spv" });
        l_ShaderSpecification.DebugName = "ShadowShader";

        auto l_Shader = Renderer::GetAPI().CreateShader(l_ShaderSpecification);

        PipelineSpecification l_PipelineSpecification{};
        l_PipelineSpecification.PipelineShader = l_Shader;
        l_PipelineSpecification.VertexAttributes = { { 0, 0, VertexAttributeFormat::Float3, offsetof(Geometry::Vertex, Position) } };
        l_PipelineSpecification.VertexStride = sizeof(Geometry::Vertex);
        l_PipelineSpecification.PushConstants = { { ShaderStage::Vertex, 0, sizeof(ShadowPushBlock) } };
        l_PipelineSpecification.ColorAttachmentFormats = {};
        l_PipelineSpecification.DepthAttachmentFormat = TextureFormat::Depth32F;
        l_PipelineSpecification.DepthTest = true;
        l_PipelineSpecification.DepthWrite = true;
        l_PipelineSpecification.DepthOp = DepthCompareOp::Less;
        l_PipelineSpecification.CullingMode = CullMode::Front;
        l_PipelineSpecification.DepthBias = true;
        l_PipelineSpecification.DepthBiasConstantFactor = m_Settings.Directional.DepthBiasConstant;
        l_PipelineSpecification.DepthBiasSlopeFactor = m_Settings.Directional.DepthBiasSlope;
        l_PipelineSpecification.DebugName = "ShadowPipeline";

        m_OpaquePipeline = Renderer::GetAPI().CreatePipeline(l_PipelineSpecification);

        TR_CORE_INFO("ShadowPass initialized");
        TR_CORE_DEBUG("ShadowPass directional cascades={} atlasResolution={}", m_Settings.Directional.CascadeCount, m_Settings.Directional.AtlasResolution);
    }

    void ShadowPass::Shutdown()
    {
        m_OpaquePipeline.reset();

        TR_CORE_INFO("ShadowPass shutdown");
    }

    void ShadowPass::SetSettings(const ShadowSettings& settings)
    {
        m_Settings = settings;

        TR_CORE_TRACE("ShadowPass settings updated directional={} spot={} point={}", m_Settings.Directional.Enabled, m_Settings.Spot.Enabled, m_Settings.Point.Enabled);
    }

    void ShadowPass::DeclareResources(RenderGraph& graph, SceneRenderGraphResources& resources)
    {
        RenderGraphTextureDescription l_AtlasDescription{};
        l_AtlasDescription.Width = m_Settings.Directional.AtlasResolution;
        l_AtlasDescription.Height = m_Settings.Directional.AtlasResolution;
        l_AtlasDescription.Format = TextureFormat::Depth32F;
        l_AtlasDescription.Usage = TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
        l_AtlasDescription.MatchSwapchainSize = false;
        l_AtlasDescription.Persistent = true;
        l_AtlasDescription.DebugName = "ShadowAtlas";

        resources.ShadowAtlas = graph.DeclareTexture(l_AtlasDescription);

        TR_CORE_DEBUG("ShadowPass declared ShadowAtlas {}x{}", l_AtlasDescription.Width, l_AtlasDescription.Height);
    }

    void ShadowPass::AddToGraph(RenderGraph& graph, SceneRenderGraphResources& resources, SceneRenderPassContext& context)
    {
        graph.AddPass("Shadow").SetType(RenderGraphPassType::Graphics).SetCullable(false).SetDebugColor(0.85f, 0.85f, 0.30f, 1.0f).Write(resources.ShadowAtlas, RenderGraphAccess::DepthStencilWrite).SetExecuteCallback([this, &resources, &context](RenderGraphContext& graphContext)
        {
            Execute(graphContext, resources, context);
        });
    }

    void ShadowPass::Execute(RenderGraphContext& context, SceneRenderGraphResources& resources, SceneRenderPassContext& passContext)
    {
        if (!m_OpaquePipeline || !passContext.ActiveCamera || !passContext.SceneData)
        {
            return;
        }

        auto l_Atlas = context.GetTexture(resources.ShadowAtlas);
        if (!l_Atlas)
        {
            return;
        }

        const uint32_t l_CascadeCount = std::min(m_Settings.Directional.CascadeCount, 4u);
        const uint32_t l_AtlasResolution = m_Settings.Directional.AtlasResolution;
        const uint32_t l_CascadeResolution = l_AtlasResolution / 2;

        float l_Splits[5] = {};
        const float l_NearClip = passContext.ActiveCamera->GetNearClip();
        const float l_FarClip = std::min(passContext.ActiveCamera->GetFarClip(), m_Settings.Directional.MaxShadowDistance);

        RendererUtilities::ComputeCascadeSplits(l_NearClip, l_FarClip, l_CascadeCount, m_Settings.Directional.SplitLambda, l_Splits);

        const glm::vec3 l_SunDirection = passContext.SceneData->HasDirectionalLight ? passContext.SceneData->SunDirection : glm::vec3(0.0f, -1.0f, 0.0f);

        for (uint32_t it_Cascade = 0; it_Cascade < l_CascadeCount; ++it_Cascade)
        {
            m_Cascades[it_Cascade] = RendererUtilities::ComputeCascadeMatrix(*passContext.ActiveCamera, l_SunDirection, l_Splits[it_Cascade], l_Splits[it_Cascade + 1], l_CascadeResolution);
        }

        CommandList& l_CommandList = context.GetCommandList();

        RenderingInfo l_RenderingInfo{};
        l_RenderingInfo.Width = l_AtlasResolution;
        l_RenderingInfo.Height = l_AtlasResolution;
        l_RenderingInfo.Depth.DepthTexture = l_Atlas;
        l_RenderingInfo.Depth.ClearOnLoad = true;
        l_RenderingInfo.Depth.ClearDepth = 1.0f;

        l_CommandList.BeginRendering(l_RenderingInfo);

        const bool l_HasShadowCasters = m_Settings.Directional.Enabled && passContext.SceneData->HasDirectionalLight && passContext.SceneData->SunCastShadows && passContext.DrawList && !passContext.DrawList->empty();

        if (l_HasShadowCasters)
        {
            l_CommandList.BindPipeline(m_OpaquePipeline);

            for (uint32_t it_Cascade = 0; it_Cascade < l_CascadeCount; ++it_Cascade)
            {
                const uint32_t l_QuadrantX = (it_Cascade % 2) * l_CascadeResolution;
                const uint32_t l_QuadrantY = (it_Cascade / 2) * l_CascadeResolution;

                l_CommandList.SetViewport(static_cast<float>(l_QuadrantX), static_cast<float>(l_QuadrantY), static_cast<float>(l_CascadeResolution), static_cast<float>(l_CascadeResolution), 0.0f, 1.0f);
                l_CommandList.SetScissor(l_QuadrantX, l_QuadrantY, l_CascadeResolution, l_CascadeResolution);

                for (const auto& it_DrawCommand : *passContext.DrawList)
                {
                    if (!it_DrawCommand.MeshRef)
                    {
                        continue;
                    }

                    auto l_VertexBuffer = it_DrawCommand.MeshRef->GetVertexBuffer();
                    auto l_IndexBuffer = it_DrawCommand.MeshRef->GetIndexBuffer();

                    if (!l_VertexBuffer || !l_IndexBuffer)
                    {
                        continue;
                    }

                    l_CommandList.BindVertexBuffer(0, l_VertexBuffer);
                    l_CommandList.BindIndexBuffer(l_IndexBuffer, 0, true);

                    ShadowPushBlock l_PushBlock{};
                    std::memcpy(&l_PushBlock.Model, it_DrawCommand.Transform, sizeof(glm::mat4));
                    l_PushBlock.LightViewProjection = m_Cascades[it_Cascade].ViewProjection;

                    l_CommandList.PushConstants(0, sizeof(ShadowPushBlock), &l_PushBlock);
                    l_CommandList.DrawIndexed(it_DrawCommand.MeshRef->GetIndexCount(), 1, 0, 0, 0);

                    if (passContext.Stats)
                    {
                        passContext.Stats->DrawCalls++;
                    }
                }
            }
        }

        l_CommandList.EndRendering();
    }
}