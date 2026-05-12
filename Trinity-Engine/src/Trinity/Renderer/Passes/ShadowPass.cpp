#include "Trinity/Renderer/Passes/ShadowPass.h"

#include "Trinity/Renderer/CommandList.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/RenderGraph/RenderGraph.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Resources/Shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

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

    void ShadowPass::Initialize()
    {
        ShaderSpecification l_ShadowShaderSpecification{};
        l_ShadowShaderSpecification.Modules.push_back({ ShaderStage::Vertex, "shadow_pass.vert.spv" });
        l_ShadowShaderSpecification.Modules.push_back({ ShaderStage::Fragment, "shadow_pass.frag.spv" });
        l_ShadowShaderSpecification.DebugName = "ShadowShader";

        auto l_ShadowShader = Renderer::GetAPI().CreateShader(l_ShadowShaderSpecification);

        PipelineSpecification l_ShadowPipelineSpecification{};
        l_ShadowPipelineSpecification.PipelineShader = l_ShadowShader;
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

        m_Pipeline = Renderer::GetAPI().CreatePipeline(l_ShadowPipelineSpecification);
    }

    void ShadowPass::Shutdown()
    {
        m_Pipeline.reset();
    }

    void ShadowPass::DeclareResources(RenderGraph& graph, SceneRenderGraphResources& resources)
    {
        RenderGraphTextureDescription l_ShadowMapDescription{};
        l_ShadowMapDescription.Width = l_ShadowMapResolution;
        l_ShadowMapDescription.Height = l_ShadowMapResolution;
        l_ShadowMapDescription.MatchSwapchainSize = false;
        l_ShadowMapDescription.Format = TextureFormat::Depth32F;
        l_ShadowMapDescription.Usage = TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
        l_ShadowMapDescription.DebugName = "ShadowMap-Depth";

        resources.ShadowMap = graph.DeclareTexture(l_ShadowMapDescription);
    }

    void ShadowPass::AddToGraph(RenderGraph& graph, SceneRenderGraphResources& resources, SceneRenderPassContext& context)
    {
        graph.AddPass("ShadowPass").SetType(RenderGraphPassType::Graphics).SetDebugColor(0.25f, 0.25f, 0.25f, 1.0f).Write(resources.ShadowMap, RenderGraphAccess::DepthStencilWrite).SetExecuteCallback([this, &resources, &context](RenderGraphContext& graphContext)
        {
            Execute(graphContext, resources, context);
        });
    }

    void ShadowPass::Execute(RenderGraphContext& context, SceneRenderGraphResources& resources, SceneRenderPassContext& passContext)
    {
        if (!m_Pipeline || !passContext.ActiveCamera || !passContext.SceneData || !passContext.DrawList)
        {
            return;
        }

        CommandList& l_CommandList = context.GetCommandList();
        auto l_ShadowMap = context.GetTexture(resources.ShadowMap);

        if (!l_ShadowMap)
        {
            return;
        }

        RenderingInfo l_RenderingInfo{};
        l_RenderingInfo.Width = l_ShadowMapResolution;
        l_RenderingInfo.Height = l_ShadowMapResolution;
        l_RenderingInfo.Depth.DepthTexture = l_ShadowMap;
        l_RenderingInfo.Depth.ClearOnLoad = true;
        l_RenderingInfo.Depth.ClearDepth = 1.0f;

        l_CommandList.BeginRendering(l_RenderingInfo);
        l_CommandList.SetViewport(0.0f, 0.0f, static_cast<float>(l_ShadowMapResolution), static_cast<float>(l_ShadowMapResolution), 0.0f, 1.0f);
        l_CommandList.SetScissor(0, 0, l_ShadowMapResolution, l_ShadowMapResolution);
        l_CommandList.SetDepthBias(1.25f, 0.0f, 1.75f);

        if (!passContext.DrawList->empty())
        {
            l_CommandList.BindPipeline(m_Pipeline);

            const glm::mat4 l_LightViewProjection = ComputeLightViewProjection(*passContext.ActiveCamera, passContext.SceneData->SunDirection, l_ShadowMapResolution);

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

                struct ShadowPushBlock
                {
                    float Model[16];
                    float LightViewProjection[16];
                } l_Push{};

                std::memcpy(l_Push.Model, it_DrawCommand.Transform, sizeof(l_Push.Model));
                std::memcpy(l_Push.LightViewProjection, glm::value_ptr(l_LightViewProjection), sizeof(l_Push.LightViewProjection));

                l_CommandList.PushConstants(0, sizeof(ShadowPushBlock), &l_Push);
                l_CommandList.DrawIndexed(it_DrawCommand.MeshRef->GetIndexCount(), 1, 0, 0, 0);
            }
        }

        l_CommandList.EndRendering();
    }
}