#include "Trinity/Renderer/Passes/DeferredLightingPass.h"

#include "Trinity/Renderer/CommandList.h"
#include "Trinity/Renderer/RenderGraph/RenderGraph.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Resources/Shader.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Trinity
{
    namespace
    {
        struct DeferredLightingPushBlock
        {
            float InverseViewProjection[16];

            float CameraPosition[4];
            float ScreenSize[4];

            float SunDirection[4];
            float SunColorIntensity[4];
        };
    }

    void DeferredLightingPass::Initialize()
    {
        DescriptorSetLayoutSpecification l_DescriptorSetLayoutSpecification{};
        l_DescriptorSetLayoutSpecification.DebugName = "DeferredLightingDescriptorSetLayout";
        l_DescriptorSetLayoutSpecification.Bindings = { { 0, DescriptorBindingType::CombinedImageSampler, ShaderStage::Fragment, 1, DescriptorBindingFlags::None }, { 1, DescriptorBindingType::CombinedImageSampler, ShaderStage::Fragment, 1, DescriptorBindingFlags::None }, { 2, DescriptorBindingType::CombinedImageSampler, ShaderStage::Fragment, 1, DescriptorBindingFlags::None }, { 3, DescriptorBindingType::CombinedImageSampler, ShaderStage::Fragment, 1, DescriptorBindingFlags::None }, { 4, DescriptorBindingType::CombinedImageSampler, ShaderStage::Fragment, 1, DescriptorBindingFlags::None } };
        m_DescriptorSetLayout = Renderer::GetAPI().CreateDescriptorSetLayout(l_DescriptorSetLayoutSpecification);

        SamplerSpecification l_SamplerSpecification{};
        l_SamplerSpecification.MinFilter = SamplerFilter::Nearest;
        l_SamplerSpecification.MagFilter = SamplerFilter::Nearest;
        l_SamplerSpecification.MipmapMode = SamplerMipmapMode::Nearest;
        l_SamplerSpecification.AddressModeU = SamplerAddressMode::ClampToEdge;
        l_SamplerSpecification.AddressModeV = SamplerAddressMode::ClampToEdge;
        l_SamplerSpecification.AddressModeW = SamplerAddressMode::ClampToEdge;
        l_SamplerSpecification.DebugName = "DeferredLightingSampler";
        m_Sampler = Renderer::GetAPI().CreateSampler(l_SamplerSpecification);

        ShaderSpecification l_ShaderSpecification{};
        l_ShaderSpecification.Modules.push_back({ ShaderStage::Vertex, "deferred_lighting.vert.spv" });
        l_ShaderSpecification.Modules.push_back({ ShaderStage::Fragment, "deferred_lighting.frag.spv" });
        l_ShaderSpecification.DebugName = "DeferredLightingShader";

        auto l_Shader = Renderer::GetAPI().CreateShader(l_ShaderSpecification);

        PipelineSpecification l_PipelineSpecification{};
        l_PipelineSpecification.PipelineShader = l_Shader;
        l_PipelineSpecification.VertexAttributes = {};
        l_PipelineSpecification.VertexStride = 0;
        l_PipelineSpecification.PushConstants = { { ShaderStage::Fragment, 0, sizeof(DeferredLightingPushBlock) } };
        l_PipelineSpecification.DescriptorSetLayouts = { m_DescriptorSetLayout };
        l_PipelineSpecification.ColorAttachmentFormats = { TextureFormat::RGBA16F };
        l_PipelineSpecification.DepthAttachmentFormat = TextureFormat::None;
        l_PipelineSpecification.DepthTest = false;
        l_PipelineSpecification.DepthWrite = false;
        l_PipelineSpecification.CullingMode = CullMode::None;
        l_PipelineSpecification.DebugName = "DeferredLightingPipeline";

        m_Pipeline = Renderer::GetAPI().CreatePipeline(l_PipelineSpecification);
    }

    void DeferredLightingPass::Shutdown()
    {
        m_DescriptorSet.reset();
        m_Sampler.reset();
        m_DescriptorSetLayout.reset();
        m_Pipeline.reset();
    }

    void DeferredLightingPass::DeclareResources(RenderGraph& graph, SceneRenderGraphResources& resources)
    {
        RenderGraphTextureDescription l_LitOutputDescription{};
        l_LitOutputDescription.Format = TextureFormat::RGBA16F;
        l_LitOutputDescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_LitOutputDescription.DebugName = "DeferredLighting-LitOutput";
        resources.LitOutput = graph.DeclareTexture(l_LitOutputDescription);
    }

    void DeferredLightingPass::AddToGraph(RenderGraph& graph, SceneRenderGraphResources& resources, SceneRenderPassContext& context)
    {
        graph.AddPass("DeferredLighting").SetType(RenderGraphPassType::Graphics).SetDebugColor(0.95f, 0.75f, 0.10f, 1.0f).Read(resources.Albedo, RenderGraphAccess::ShaderSampledRead).Read(resources.Normal, RenderGraphAccess::ShaderSampledRead).Read(resources.MetallicRoughnessAO, RenderGraphAccess::ShaderSampledRead).Read(resources.Depth, RenderGraphAccess::ShaderSampledRead).Write(resources.LitOutput, RenderGraphAccess::ColorAttachmentWrite).SetExecuteCallback([this, &resources, &context](RenderGraphContext& graphContext)
        {
            Execute(graphContext, resources, context);
        });
    }

    void DeferredLightingPass::WriteDescriptors(SceneRenderGraphResources& resources, RenderGraphContext& context)
    {
        if (!m_DescriptorSetLayout || !m_Sampler)
        {
            return;
        }

        auto l_Albedo = context.GetTexture(resources.Albedo);
        auto l_Normal = context.GetTexture(resources.Normal);
        auto l_MetallicRoughnessAO = context.GetTexture(resources.MetallicRoughnessAO);
        auto l_Depth = context.GetTexture(resources.Depth);

        if (!l_Albedo || !l_Normal || !l_MetallicRoughnessAO || !l_Depth)
        {
            return;
        }

        m_DescriptorSet = Renderer::GetAPI().AllocateTransientDescriptorSet(m_DescriptorSetLayout);
        m_DescriptorSet->WriteSampledImage(0, l_Albedo, m_Sampler);
        m_DescriptorSet->WriteSampledImage(1, l_Normal, m_Sampler);
        m_DescriptorSet->WriteSampledImage(2, l_MetallicRoughnessAO, m_Sampler);
        m_DescriptorSet->WriteSampledImage(3, l_Depth, m_Sampler);
        m_DescriptorSet->Flush();
    }

    void DeferredLightingPass::Execute(RenderGraphContext& context, SceneRenderGraphResources& resources, SceneRenderPassContext& passContext)
    {
        if (!m_Pipeline || !passContext.SceneData || !passContext.ActiveCamera)
        {
            return;
        }

        auto l_LitOutput = context.GetTexture(resources.LitOutput);

        if (!l_LitOutput)
        {
            return;
        }

        WriteDescriptors(resources, context);

        if (!m_DescriptorSet)
        {
            return;
        }

        RenderingInfo l_RenderingInfo{};
        l_RenderingInfo.Width = passContext.Width;
        l_RenderingInfo.Height = passContext.Height;
        l_RenderingInfo.ColorAttachments.resize(1);
        l_RenderingInfo.ColorAttachments[0].ColorTexture = l_LitOutput;
        l_RenderingInfo.ColorAttachments[0].ClearOnLoad = true;
        l_RenderingInfo.ColorAttachments[0].ClearColor[0] = 0.01f;
        l_RenderingInfo.ColorAttachments[0].ClearColor[1] = 0.01f;
        l_RenderingInfo.ColorAttachments[0].ClearColor[2] = 0.01f;
        l_RenderingInfo.ColorAttachments[0].ClearColor[3] = 1.0f;

        CommandList& l_CommandList = context.GetCommandList();

        l_CommandList.BeginRendering(l_RenderingInfo);
        l_CommandList.SetViewport(0.0f, 0.0f, static_cast<float>(passContext.Width), static_cast<float>(passContext.Height), 0.0f, 1.0f);
        l_CommandList.SetScissor(0, 0, passContext.Width, passContext.Height);
        l_CommandList.BindPipeline(m_Pipeline);
        l_CommandList.BindDescriptorSet(0, m_DescriptorSet);

        DeferredLightingPushBlock l_PushBlock{};

        const glm::mat4 l_InverseViewProjection = glm::inverse(passContext.ActiveCamera->GetViewProjectionMatrix());
        const float* l_InverseViewProjectionData = glm::value_ptr(l_InverseViewProjection);

        for (uint32_t i = 0; i < 16; i++)
        {
            l_PushBlock.InverseViewProjection[i] = l_InverseViewProjectionData[i];
        }

        const glm::vec3& l_CameraPosition = passContext.ActiveCamera->GetPosition();

        l_PushBlock.CameraPosition[0] = l_CameraPosition.x;
        l_PushBlock.CameraPosition[1] = l_CameraPosition.y;
        l_PushBlock.CameraPosition[2] = l_CameraPosition.z;
        l_PushBlock.CameraPosition[3] = 1.0f;

        l_PushBlock.ScreenSize[0] = static_cast<float>(passContext.Width);
        l_PushBlock.ScreenSize[1] = static_cast<float>(passContext.Height);
        l_PushBlock.ScreenSize[2] = passContext.Width > 0 ? 1.0f / static_cast<float>(passContext.Width) : 0.0f;
        l_PushBlock.ScreenSize[3] = passContext.Height > 0 ? 1.0f / static_cast<float>(passContext.Height) : 0.0f;

        l_PushBlock.SunDirection[0] = passContext.SceneData->SunDirection.x;
        l_PushBlock.SunDirection[1] = passContext.SceneData->SunDirection.y;
        l_PushBlock.SunDirection[2] = passContext.SceneData->SunDirection.z;
        l_PushBlock.SunDirection[3] = 0.0f;

        l_PushBlock.SunColorIntensity[0] = passContext.SceneData->SunLight.Color.r;
        l_PushBlock.SunColorIntensity[1] = passContext.SceneData->SunLight.Color.g;
        l_PushBlock.SunColorIntensity[2] = passContext.SceneData->SunLight.Color.b;
        l_PushBlock.SunColorIntensity[3] = passContext.SceneData->HasDirectionalLight ? passContext.SceneData->SunLight.Intensity : 0.0f;

        l_CommandList.PushConstants(0, sizeof(DeferredLightingPushBlock), &l_PushBlock);
        l_CommandList.Draw(3, 1, 0, 0);
        l_CommandList.EndRendering();
    }
}