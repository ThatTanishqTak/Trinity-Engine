#include "Trinity/Renderer/Passes/ShadowPass.h"

#include "Trinity/Renderer/CommandList.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/RenderGraph/RenderGraph.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Renderer/Resources/Texture.h"
#include "Trinity/Utilities/Log.h"

#include <glm/glm.hpp>

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
        (void)context;

        graph.AddPass("Shadow").SetType(RenderGraphPassType::Graphics).SetCullable(false).SetDebugColor(0.85f, 0.85f, 0.30f, 1.0f).Write(resources.ShadowAtlas, RenderGraphAccess::DepthStencilWrite).SetExecuteCallback([this, &resources](RenderGraphContext& graphContext)
        {
            auto l_Atlas = graphContext.GetTexture(resources.ShadowAtlas);
            if (!l_Atlas)
            {
                return;
            }

            CommandList& l_CommandList = graphContext.GetCommandList();

            RenderingInfo l_RenderingInfo{};
            l_RenderingInfo.Width = m_Settings.Directional.AtlasResolution;
            l_RenderingInfo.Height = m_Settings.Directional.AtlasResolution;
            l_RenderingInfo.Depth.DepthTexture = l_Atlas;
            l_RenderingInfo.Depth.ClearOnLoad = true;
            l_RenderingInfo.Depth.ClearDepth = 1.0f;

            l_CommandList.BeginRendering(l_RenderingInfo);
            l_CommandList.EndRendering();
        });
    }
}