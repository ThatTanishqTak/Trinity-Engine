#include "Trinity/Renderer/Passes/GeometryPass.h"

#include "Trinity/Renderer/CommandList.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/RenderGraph/RenderGraph.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Resources/Shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace Trinity
{
    namespace
    {
        std::shared_ptr<Texture> CreateWhiteTexture()
        {
            TextureSpecification l_TextureSpecification{};
            l_TextureSpecification.Width = 1;
            l_TextureSpecification.Height = 1;
            l_TextureSpecification.Format = TextureFormat::RGBA8;
            l_TextureSpecification.Usage = TextureUsage::Sampled | TextureUsage::TransferDestination;
            l_TextureSpecification.DebugName = "DefaultWhiteTexture";

            auto l_Texture = Renderer::GetAPI().CreateTexture(l_TextureSpecification);

            if (l_Texture)
            {
                constexpr uint32_t l_WhitePixel = 0xFFFFFFFFu;
                l_Texture->Upload(&l_WhitePixel, sizeof(l_WhitePixel));
            }

            return l_Texture;
        }
    }

    void GeometryPass::Initialize()
    {
        DescriptorSetLayoutSpecification l_DescriptorSetLayoutSpecification{};
        l_DescriptorSetLayoutSpecification.DebugName = "GeometryMaterialDescriptorSetLayout";
        l_DescriptorSetLayoutSpecification.Bindings = { { 0, DescriptorBindingType::CombinedImageSampler, ShaderStage::Fragment, 1, DescriptorBindingFlags::None } };
        m_DescriptorSetLayout = Renderer::GetAPI().CreateDescriptorSetLayout(l_DescriptorSetLayoutSpecification);

        SamplerSpecification l_SamplerSpecification{};
        l_SamplerSpecification.MinFilter = SamplerFilter::Linear;
        l_SamplerSpecification.MagFilter = SamplerFilter::Linear;
        l_SamplerSpecification.MipmapMode = SamplerMipmapMode::Linear;
        l_SamplerSpecification.AddressModeU = SamplerAddressMode::Repeat;
        l_SamplerSpecification.AddressModeV = SamplerAddressMode::Repeat;
        l_SamplerSpecification.AddressModeW = SamplerAddressMode::Repeat;
        l_SamplerSpecification.DebugName = "GeometryMaterialSampler";
        m_Sampler = Renderer::GetAPI().CreateSampler(l_SamplerSpecification);

        m_WhiteTexture = CreateWhiteTexture();

        ShaderSpecification l_GeometryShaderSpecification{};
        l_GeometryShaderSpecification.Modules.push_back({ ShaderStage::Vertex, "geometry_pass.vert.spv" });
        l_GeometryShaderSpecification.Modules.push_back({ ShaderStage::Fragment, "geometry_pass.frag.spv" });
        l_GeometryShaderSpecification.DebugName = "MeshShader";

        auto l_Shader = Renderer::GetAPI().CreateShader(l_GeometryShaderSpecification);

        PipelineSpecification l_GeometryPipelineSpecification{};
        l_GeometryPipelineSpecification.PipelineShader = l_Shader;
        l_GeometryPipelineSpecification.VertexAttributes = { { 0, 0, VertexAttributeFormat::Float3, offsetof(Geometry::Vertex, Position) }, { 1, 0, VertexAttributeFormat::Float3, offsetof(Geometry::Vertex, Normal) }, { 2, 0, VertexAttributeFormat::Float2, offsetof(Geometry::Vertex, UV) }, { 3, 0, VertexAttributeFormat::Float3, offsetof(Geometry::Vertex, Tangent) } };
        l_GeometryPipelineSpecification.VertexStride = sizeof(Geometry::Vertex);
        l_GeometryPipelineSpecification.PushConstants = { { ShaderStage::Vertex, 0, sizeof(PushBlock) }, { ShaderStage::Fragment, 0, sizeof(PushBlock) } };
        l_GeometryPipelineSpecification.DescriptorSetLayouts = { m_DescriptorSetLayout };
        l_GeometryPipelineSpecification.ColorAttachmentFormats = { TextureFormat::RGBA8, TextureFormat::RGBA16F, TextureFormat::RGBA8 };
        l_GeometryPipelineSpecification.DepthAttachmentFormat = TextureFormat::Depth32F;
        l_GeometryPipelineSpecification.DepthTest = true;
        l_GeometryPipelineSpecification.DepthWrite = true;
        l_GeometryPipelineSpecification.DepthOp = DepthCompareOp::Less;
        l_GeometryPipelineSpecification.CullingMode = CullMode::None;
        l_GeometryPipelineSpecification.DebugName = "MeshPipeline";

        m_Pipeline = Renderer::GetAPI().CreatePipeline(l_GeometryPipelineSpecification);
    }

    void GeometryPass::Shutdown()
    {
        m_DescriptorSet.reset();
        m_WhiteTexture.reset();
        m_Sampler.reset();
        m_DescriptorSetLayout.reset();
        m_Pipeline.reset();
    }

    void GeometryPass::DeclareResources(RenderGraph& graph, SceneRenderGraphResources& resources)
    {
        RenderGraphTextureDescription l_AlbedoDescription{};
        l_AlbedoDescription.Format = TextureFormat::RGBA8;
        l_AlbedoDescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_AlbedoDescription.DebugName = "GeometryBuffer-Albedo";
        resources.Albedo = graph.DeclareTexture(l_AlbedoDescription);

        RenderGraphTextureDescription l_NormalDescription{};
        l_NormalDescription.Format = TextureFormat::RGBA16F;
        l_NormalDescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_NormalDescription.DebugName = "GeometryBuffer-Normal";
        resources.Normal = graph.DeclareTexture(l_NormalDescription);

        RenderGraphTextureDescription l_MetallicRoughnessAODescription{};
        l_MetallicRoughnessAODescription.Format = TextureFormat::RGBA8;
        l_MetallicRoughnessAODescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_MetallicRoughnessAODescription.DebugName = "GeometryBuffer-MR-AO";
        resources.MetallicRoughnessAO = graph.DeclareTexture(l_MetallicRoughnessAODescription);

        RenderGraphTextureDescription l_DepthDescription{};
        l_DepthDescription.Format = TextureFormat::Depth32F;
        l_DepthDescription.Usage = TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
        l_DepthDescription.DebugName = "GeometryBuffer-Depth";
        resources.Depth = graph.DeclareTexture(l_DepthDescription);
    }

    void GeometryPass::AddToGraph(RenderGraph& graph, SceneRenderGraphResources& resources, SceneRenderPassContext& context)
    {
        graph.AddPass("Geometry").SetType(RenderGraphPassType::Graphics).SetDebugColor(0.15f, 0.35f, 0.95f, 1.0f).Read(resources.ShadowMap, RenderGraphAccess::ShaderSampledRead).Write(resources.Albedo, RenderGraphAccess::ColorAttachmentWrite).Write(resources.Normal, RenderGraphAccess::ColorAttachmentWrite).Write(resources.MetallicRoughnessAO, RenderGraphAccess::ColorAttachmentWrite).Write(resources.Depth, RenderGraphAccess::DepthStencilWrite).SetExecuteCallback([this, &resources, &context](RenderGraphContext& graphContext)
        {
            Execute(graphContext, resources, context);
        });
    }

    void GeometryPass::WriteMaterialDescriptors(RenderGraphContext& context, const MeshDrawCommand& drawCommand)
    {
        (void)context;

        if (!m_DescriptorSetLayout || !m_Sampler)
        {
            return;
        }

        std::shared_ptr<Texture> l_AlbedoTexture = m_WhiteTexture;

        if (drawCommand.AlbedoTexture)
        {
            l_AlbedoTexture = drawCommand.AlbedoTexture;
        }

        if (!l_AlbedoTexture)
        {
            return;
        }

        m_DescriptorSet = Renderer::GetAPI().AllocateTransientDescriptorSet(m_DescriptorSetLayout);
        m_DescriptorSet->WriteSampledImage(0, l_AlbedoTexture, m_Sampler);
        m_DescriptorSet->Flush();
    }

    void GeometryPass::Execute(RenderGraphContext& context, SceneRenderGraphResources& resources, SceneRenderPassContext& passContext)
    {
        if (!m_Pipeline || !m_DescriptorSetLayout || !m_Sampler || !passContext.ActiveCamera || !passContext.DrawList || !passContext.Stats)
        {
            return;
        }

        CommandList& l_CommandList = context.GetCommandList();

        auto l_Albedo = context.GetTexture(resources.Albedo);
        auto l_Normal = context.GetTexture(resources.Normal);
        auto l_MetallicRoughnessAO = context.GetTexture(resources.MetallicRoughnessAO);
        auto l_Depth = context.GetTexture(resources.Depth);

        if (!l_Albedo || !l_Normal || !l_MetallicRoughnessAO || !l_Depth)
        {
            return;
        }

        RenderingInfo l_RenderingInfo{};
        l_RenderingInfo.Width = passContext.Width;
        l_RenderingInfo.Height = passContext.Height;
        l_RenderingInfo.ColorAttachments.resize(3);

        l_RenderingInfo.ColorAttachments[0].ColorTexture = l_Albedo;
        l_RenderingInfo.ColorAttachments[0].ClearOnLoad = true;
        l_RenderingInfo.ColorAttachments[0].ClearColor[0] = 0.01f;
        l_RenderingInfo.ColorAttachments[0].ClearColor[1] = 0.01f;
        l_RenderingInfo.ColorAttachments[0].ClearColor[2] = 0.01f;
        l_RenderingInfo.ColorAttachments[0].ClearColor[3] = 1.0f;

        l_RenderingInfo.ColorAttachments[1].ColorTexture = l_Normal;
        l_RenderingInfo.ColorAttachments[1].ClearOnLoad = true;
        l_RenderingInfo.ColorAttachments[1].ClearColor[0] = 0.0f;
        l_RenderingInfo.ColorAttachments[1].ClearColor[1] = 0.0f;
        l_RenderingInfo.ColorAttachments[1].ClearColor[2] = 0.0f;
        l_RenderingInfo.ColorAttachments[1].ClearColor[3] = 0.0f;

        l_RenderingInfo.ColorAttachments[2].ColorTexture = l_MetallicRoughnessAO;
        l_RenderingInfo.ColorAttachments[2].ClearOnLoad = true;
        l_RenderingInfo.ColorAttachments[2].ClearColor[0] = 0.0f;
        l_RenderingInfo.ColorAttachments[2].ClearColor[1] = 0.5f;
        l_RenderingInfo.ColorAttachments[2].ClearColor[2] = 1.0f;
        l_RenderingInfo.ColorAttachments[2].ClearColor[3] = 0.0f;

        l_RenderingInfo.Depth.DepthTexture = l_Depth;
        l_RenderingInfo.Depth.ClearOnLoad = true;
        l_RenderingInfo.Depth.ClearDepth = 1.0f;

        l_CommandList.BeginRendering(l_RenderingInfo);
        l_CommandList.SetViewport(0.0f, 0.0f, static_cast<float>(passContext.Width), static_cast<float>(passContext.Height), 0.0f, 1.0f);
        l_CommandList.SetScissor(0, 0, passContext.Width, passContext.Height);

        if (!passContext.DrawList->empty())
        {
            l_CommandList.BindPipeline(m_Pipeline);

            const glm::mat4 l_ViewProjection = passContext.ActiveCamera->GetViewProjectionMatrix();

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

                WriteMaterialDescriptors(context, it_DrawCommand);

                if (!m_DescriptorSet)
                {
                    continue;
                }

                l_CommandList.BindDescriptorSet(0, m_DescriptorSet);
                l_CommandList.BindVertexBuffer(0, l_VertexBuffer);
                l_CommandList.BindIndexBuffer(l_IndexBuffer, 0, true);

                PushBlock l_PushBlock{};

                l_PushBlock.BaseColor[0] = it_DrawCommand.Material.BaseColor.r;
                l_PushBlock.BaseColor[1] = it_DrawCommand.Material.BaseColor.g;
                l_PushBlock.BaseColor[2] = it_DrawCommand.Material.BaseColor.b;
                l_PushBlock.BaseColor[3] = it_DrawCommand.Material.BaseColor.a;

                l_PushBlock.MaterialData[0] = it_DrawCommand.Material.Metallic;
                l_PushBlock.MaterialData[1] = it_DrawCommand.Material.Roughness;
                l_PushBlock.MaterialData[2] = it_DrawCommand.Material.AmbientOcclusion;
                l_PushBlock.MaterialData[3] = static_cast<float>(it_DrawCommand.Material.AlphaMode);

                l_PushBlock.EmissiveColorStrength[0] = it_DrawCommand.Material.EmissiveColor.r;
                l_PushBlock.EmissiveColorStrength[1] = it_DrawCommand.Material.EmissiveColor.g;
                l_PushBlock.EmissiveColorStrength[2] = it_DrawCommand.Material.EmissiveColor.b;
                l_PushBlock.EmissiveColorStrength[3] = it_DrawCommand.Material.EmissiveStrength;

                l_PushBlock.TextureFlags[0] = it_DrawCommand.UseAlbedoTexture ? 1.0f : 0.0f;
                l_PushBlock.TextureFlags[1] = 0.0f;
                l_PushBlock.TextureFlags[2] = 0.0f;
                l_PushBlock.TextureFlags[3] = 0.0f;

                std::memcpy(l_PushBlock.Model, it_DrawCommand.Transform, sizeof(l_PushBlock.Model));
                std::memcpy(l_PushBlock.ViewProjection, glm::value_ptr(l_ViewProjection), sizeof(l_PushBlock.ViewProjection));

                l_CommandList.PushConstants(0, sizeof(PushBlock), &l_PushBlock);
                l_CommandList.DrawIndexed(it_DrawCommand.MeshRef->GetIndexCount(), 1, 0, 0, 0);

                passContext.Stats->DrawCalls++;
                passContext.Stats->IndexCount += it_DrawCommand.MeshRef->GetIndexCount();
                passContext.Stats->VertexCount += it_DrawCommand.MeshRef->GetVertexCount();
            }
        }

        l_CommandList.EndRendering();
    }
}