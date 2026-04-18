#include "Trinity/Renderer/SceneRenderer.h"

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"
#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#include "Trinity/Renderer/Vulkan/RenderGraph/VulkanRenderGraph.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Utilities/Log.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <cstring>
#include <memory>

namespace Trinity
{
    struct SceneRenderer::Implementation
    {
        SceneRendererStats Stats;
        bool SceneActive = false;
        std::unique_ptr<VulkanRenderGraph> RenderGraph;

        std::shared_ptr<Pipeline> GeometryPipeline;
        std::shared_ptr<Pipeline> ShadowPipeline;

        RenderGraphResourceHandle AlbedoHandle;
        RenderGraphResourceHandle NormalHandle;
        RenderGraphResourceHandle MRAHandle;
        RenderGraphResourceHandle DepthHandle;
        RenderGraphResourceHandle ShadowHandle;
    };

    SceneRenderer::SceneRenderer() = default;
    SceneRenderer::~SceneRenderer() = default;

    void SceneRenderer::Initialize(uint32_t width, uint32_t height)
    {
        m_Width  = width;
        m_Height = height;
        m_Implementation = std::make_unique<Implementation>();

        auto* a_VkAPI = dynamic_cast<VulkanRendererAPI*>(&Renderer::GetAPI());
        if (!a_VkAPI)
        {
            TR_CORE_ERROR("SceneRenderer requires Vulkan backend");
            return;
        }

        m_Implementation->RenderGraph = std::make_unique<VulkanRenderGraph>(*a_VkAPI);

        constexpr uint32_t l_ShadowMapSize = 2048;

        RenderGraphTextureDescription l_AlbedoDesc{};
        l_AlbedoDesc.Format    = TextureFormat::RGBA8;
        l_AlbedoDesc.Usage     = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_AlbedoDesc.DebugName = "GBuffer-Albedo";
        m_Implementation->AlbedoHandle = m_Implementation->RenderGraph->DeclareTexture(l_AlbedoDesc);

        RenderGraphTextureDescription l_NormalDesc{};
        l_NormalDesc.Format    = TextureFormat::RGBA16F;
        l_NormalDesc.Usage     = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_NormalDesc.DebugName = "GBuffer-Normal";
        m_Implementation->NormalHandle = m_Implementation->RenderGraph->DeclareTexture(l_NormalDesc);

        RenderGraphTextureDescription l_MRADesc{};
        l_MRADesc.Format    = TextureFormat::RGBA8;
        l_MRADesc.Usage     = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_MRADesc.DebugName = "GBuffer-MRA";
        m_Implementation->MRAHandle = m_Implementation->RenderGraph->DeclareTexture(l_MRADesc);

        RenderGraphTextureDescription l_DepthDesc{};
        l_DepthDesc.Format    = TextureFormat::Depth32F;
        l_DepthDesc.Usage     = TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
        l_DepthDesc.DebugName = "GBuffer-Depth";
        m_Implementation->DepthHandle = m_Implementation->RenderGraph->DeclareTexture(l_DepthDesc);

        RenderGraphTextureDescription l_ShadowDesc{};
        l_ShadowDesc.Width     = l_ShadowMapSize;
        l_ShadowDesc.Height    = l_ShadowMapSize;
        l_ShadowDesc.Format    = TextureFormat::Depth32F;
        l_ShadowDesc.Usage     = TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
        l_ShadowDesc.DebugName = "ShadowMap";
        m_Implementation->ShadowHandle = m_Implementation->RenderGraph->DeclareTexture(l_ShadowDesc);

        const auto l_AlbedoHandle = m_Implementation->AlbedoHandle;
        const auto l_NormalHandle = m_Implementation->NormalHandle;
        const auto l_MRAHandle    = m_Implementation->MRAHandle;
        const auto l_DepthHandle  = m_Implementation->DepthHandle;
        const auto l_ShadowHandle = m_Implementation->ShadowHandle;

        m_Implementation->RenderGraph->AddPass("Shadow")
            .Write(l_ShadowHandle)
            .SetExecuteCallback([this, a_VkAPI, l_ShadowHandle](RenderGraphContext& ctx)
            {
                VkCommandBuffer l_Cmd = a_VkAPI->GetCurrentCommandBuffer();

                auto l_ShadowTex = std::dynamic_pointer_cast<VulkanTexture>(ctx.GetTexture(l_ShadowHandle));
                if (!l_ShadowTex)
                {
                    return;
                }

                VkRenderingAttachmentInfo l_DepthAttachment{};
                l_DepthAttachment.sType                   = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                l_DepthAttachment.imageView               = l_ShadowTex->GetImageView();
                l_DepthAttachment.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                l_DepthAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
                l_DepthAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
                l_DepthAttachment.clearValue.depthStencil = { 1.0f, 0 };

                constexpr uint32_t l_Size = 2048;

                VkRenderingInfo l_RenderingInfo{};
                l_RenderingInfo.sType            = VK_STRUCTURE_TYPE_RENDERING_INFO;
                l_RenderingInfo.renderArea       = { { 0, 0 }, { l_Size, l_Size } };
                l_RenderingInfo.layerCount       = 1;
                l_RenderingInfo.pDepthAttachment = &l_DepthAttachment;
                vkCmdBeginRendering(l_Cmd, &l_RenderingInfo);

                VkViewport l_Viewport{};
                l_Viewport.width    = static_cast<float>(l_Size);
                l_Viewport.height   = static_cast<float>(l_Size);
                l_Viewport.minDepth = 0.0f;
                l_Viewport.maxDepth = 1.0f;
                vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

                VkRect2D l_Scissor{};
                l_Scissor.extent = { l_Size, l_Size };
                vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

                if (!m_DrawList.empty() && m_Implementation->ShadowPipeline)
                {
                    auto* a_VkPipeline = dynamic_cast<VulkanPipeline*>(m_Implementation->ShadowPipeline.get());
                    vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, a_VkPipeline->GetPipeline());

                    glm::vec3 l_LightDir = glm::normalize(glm::vec3(
                        m_SceneData.SunLight.Direction[0],
                        m_SceneData.SunLight.Direction[1],
                        m_SceneData.SunLight.Direction[2]
                    ));

                    const glm::vec3 l_Up = (glm::abs(glm::dot(l_LightDir, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f)
                        ? glm::vec3(0.0f, 0.0f, 1.0f)
                        : glm::vec3(0.0f, 1.0f, 0.0f);

                    glm::mat4 l_LightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
                    l_LightProjection[1][1] *= -1.0f;
                    const glm::mat4 l_LightView = glm::lookAt(-l_LightDir * 30.0f, glm::vec3(0.0f), l_Up);
                    const glm::mat4 l_LightSpaceMatrix = l_LightProjection * l_LightView;

                    for (const auto& cmd : m_DrawList)
                    {
                        if (!cmd.MeshRef) continue;

                        auto* a_VB = dynamic_cast<VulkanBuffer*>(cmd.MeshRef->GetVertexBuffer().get());
                        auto* a_IB = dynamic_cast<VulkanBuffer*>(cmd.MeshRef->GetIndexBuffer().get());
                        if (!a_VB || !a_IB) continue;

                        VkBuffer l_Buffers[]     = { a_VB->GetBuffer() };
                        VkDeviceSize l_Offsets[] = { 0 };
                        vkCmdBindVertexBuffers(l_Cmd, 0, 1, l_Buffers, l_Offsets);
                        vkCmdBindIndexBuffer(l_Cmd, a_IB->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

                        struct { float LightSpaceMatrix[16]; float Model[16]; } l_Push;
                        std::memcpy(l_Push.LightSpaceMatrix, glm::value_ptr(l_LightSpaceMatrix), 64);
                        std::memcpy(l_Push.Model,            cmd.Transform,                      64);
                        vkCmdPushConstants(l_Cmd, a_VkPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 128, &l_Push);

                        vkCmdDrawIndexed(l_Cmd, cmd.MeshRef->GetIndexCount(), 1, 0, 0, 0);
                    }
                }

                vkCmdEndRendering(l_Cmd);
            });

        m_Implementation->RenderGraph->AddPass("Geometry")
            .Read(l_ShadowHandle)
            .Write(l_AlbedoHandle)
            .Write(l_NormalHandle)
            .Write(l_MRAHandle)
            .Write(l_DepthHandle)
            .SetExecuteCallback([this, a_VkAPI, l_AlbedoHandle, l_NormalHandle, l_MRAHandle, l_DepthHandle](RenderGraphContext& ctx)
            {
                VkCommandBuffer l_Cmd = a_VkAPI->GetCurrentCommandBuffer();

                auto l_Albedo = std::dynamic_pointer_cast<VulkanTexture>(ctx.GetTexture(l_AlbedoHandle));
                auto l_Normal = std::dynamic_pointer_cast<VulkanTexture>(ctx.GetTexture(l_NormalHandle));
                auto l_MRA    = std::dynamic_pointer_cast<VulkanTexture>(ctx.GetTexture(l_MRAHandle));
                auto l_Depth  = std::dynamic_pointer_cast<VulkanTexture>(ctx.GetTexture(l_DepthHandle));

                if (!l_Albedo || !l_Normal || !l_MRA || !l_Depth)
                {
                    return;
                }

                auto a_MakeColorAttachment = [](VkImageView view) -> VkRenderingAttachmentInfo
                {
                    VkRenderingAttachmentInfo l_Info{};
                    l_Info.sType            = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                    l_Info.imageView        = view;
                    l_Info.imageLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    l_Info.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    l_Info.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
                    l_Info.clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
                    return l_Info;
                };

                std::array<VkRenderingAttachmentInfo, 3> l_ColorAttachments = {
                    a_MakeColorAttachment(l_Albedo->GetImageView()),
                    a_MakeColorAttachment(l_Normal->GetImageView()),
                    a_MakeColorAttachment(l_MRA->GetImageView())
                };

                VkRenderingAttachmentInfo l_DepthAttachment{};
                l_DepthAttachment.sType                   = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                l_DepthAttachment.imageView               = l_Depth->GetImageView();
                l_DepthAttachment.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                l_DepthAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
                l_DepthAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
                l_DepthAttachment.clearValue.depthStencil = { 1.0f, 0 };

                VkRenderingInfo l_RenderingInfo{};
                l_RenderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
                l_RenderingInfo.renderArea           = { { 0, 0 }, { m_Width, m_Height } };
                l_RenderingInfo.layerCount           = 1;
                l_RenderingInfo.colorAttachmentCount = static_cast<uint32_t>(l_ColorAttachments.size());
                l_RenderingInfo.pColorAttachments    = l_ColorAttachments.data();
                l_RenderingInfo.pDepthAttachment     = &l_DepthAttachment;
                vkCmdBeginRendering(l_Cmd, &l_RenderingInfo);

                VkViewport l_Viewport{};
                l_Viewport.width    = static_cast<float>(m_Width);
                l_Viewport.height   = static_cast<float>(m_Height);
                l_Viewport.minDepth = 0.0f;
                l_Viewport.maxDepth = 1.0f;
                vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

                VkRect2D l_Scissor{};
                l_Scissor.extent = { m_Width, m_Height };
                vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

                if (!m_DrawList.empty())
                {
                    auto* a_VkPipeline = dynamic_cast<VulkanPipeline*>(m_Implementation->GeometryPipeline.get());
                    vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, a_VkPipeline->GetPipeline());

                    const glm::mat4 l_VP = m_Camera.GetViewProjectionMatrix();

                    for (const auto& cmd : m_DrawList)
                    {
                        if (!cmd.MeshRef) continue;

                        auto* a_VB = dynamic_cast<VulkanBuffer*>(cmd.MeshRef->GetVertexBuffer().get());
                        auto* a_IB = dynamic_cast<VulkanBuffer*>(cmd.MeshRef->GetIndexBuffer().get());
                        if (!a_VB || !a_IB) continue;

                        VkBuffer l_Buffers[]     = { a_VB->GetBuffer() };
                        VkDeviceSize l_Offsets[] = { 0 };
                        vkCmdBindVertexBuffers(l_Cmd, 0, 1, l_Buffers, l_Offsets);
                        vkCmdBindIndexBuffer(l_Cmd, a_IB->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

                        struct { float Model[16]; float VP[16]; } l_Push;
                        std::memcpy(l_Push.Model, cmd.Transform,        64);
                        std::memcpy(l_Push.VP,    glm::value_ptr(l_VP), 64);
                        vkCmdPushConstants(l_Cmd, a_VkPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 128, &l_Push);

                        vkCmdDrawIndexed(l_Cmd, cmd.MeshRef->GetIndexCount(), 1, 0, 0, 0);

                        m_Implementation->Stats.DrawCalls++;
                        m_Implementation->Stats.IndexCount  += cmd.MeshRef->GetIndexCount();
                        m_Implementation->Stats.VertexCount += cmd.MeshRef->GetVertexCount();
                    }
                }

                vkCmdEndRendering(l_Cmd);
            });

        m_Implementation->RenderGraph->AddPass("Output")
            .Read(l_AlbedoHandle);

        ShaderSpecification l_ShadowShaderSpec{};
        l_ShadowShaderSpec.Modules.push_back({ ShaderStage::Vertex,   "shadow_pass.vert.spv" });
        l_ShadowShaderSpec.Modules.push_back({ ShaderStage::Fragment, "shadow_pass.frag.spv" });
        l_ShadowShaderSpec.DebugName = "ShadowShader";
        auto l_ShadowShader = Renderer::GetAPI().CreateShader(l_ShadowShaderSpec);

        PipelineSpecification l_ShadowPipelineSpec{};
        l_ShadowPipelineSpec.PipelineShader = l_ShadowShader;
        l_ShadowPipelineSpec.VertexAttributes = {
            { 0, 0, VertexAttributeFormat::Float3, 0 },
        };
        l_ShadowPipelineSpec.VertexStride            = sizeof(Vertex);
        l_ShadowPipelineSpec.PushConstants           = {{ ShaderStage::Vertex, 0, 128 }};
        l_ShadowPipelineSpec.DepthAttachmentFormat   = TextureFormat::Depth32F;
        l_ShadowPipelineSpec.DepthTest               = true;
        l_ShadowPipelineSpec.DepthWrite              = true;
        l_ShadowPipelineSpec.DepthOp                 = DepthCompareOp::Less;
        l_ShadowPipelineSpec.CullingMode             = CullMode::Back;
        l_ShadowPipelineSpec.DepthBias               = true;
        l_ShadowPipelineSpec.DepthBiasConstantFactor = 1.25f;
        l_ShadowPipelineSpec.DepthBiasSlopeFactor    = 1.75f;
        l_ShadowPipelineSpec.DebugName               = "ShadowPipeline";
        m_Implementation->ShadowPipeline = Renderer::GetAPI().CreatePipeline(l_ShadowPipelineSpec);

        ShaderSpecification l_ShaderSpec{};
        l_ShaderSpec.Modules.push_back({ ShaderStage::Vertex,   "geometry_pass.vert.spv" });
        l_ShaderSpec.Modules.push_back({ ShaderStage::Fragment, "geometry_pass.frag.spv" });
        l_ShaderSpec.DebugName = "GeometryShader";
        auto l_Shader = Renderer::GetAPI().CreateShader(l_ShaderSpec);

        PipelineSpecification l_PipelineSpec{};
        l_PipelineSpec.PipelineShader     = l_Shader;
        l_PipelineSpec.VertexAttributes   = {
            { 0, 0, VertexAttributeFormat::Float3,  0 },
            { 1, 0, VertexAttributeFormat::Float3, 12 },
            { 2, 0, VertexAttributeFormat::Float2, 24 },
        };
        l_PipelineSpec.VertexStride           = sizeof(Vertex);
        l_PipelineSpec.PushConstants          = {{ ShaderStage::Vertex, 0, 128 }};
        l_PipelineSpec.ColorAttachmentFormats = { TextureFormat::RGBA8, TextureFormat::RGBA16F, TextureFormat::RGBA8 };
        l_PipelineSpec.DepthAttachmentFormat  = TextureFormat::Depth32F;
        l_PipelineSpec.DepthTest              = true;
        l_PipelineSpec.DepthWrite             = true;
        l_PipelineSpec.DepthOp                = DepthCompareOp::Less;
        l_PipelineSpec.CullingMode            = CullMode::Back;
        l_PipelineSpec.DebugName              = "GeometryPipeline";
        m_Implementation->GeometryPipeline = Renderer::GetAPI().CreatePipeline(l_PipelineSpec);
    }

    void SceneRenderer::Shutdown()
    {
        m_Implementation.reset();
    }

    void SceneRenderer::BeginScene(const Camera& camera, const SceneRenderData& sceneData)
    {
        m_Camera    = camera;
        m_SceneData = sceneData;
        m_DrawList.clear();
        m_Implementation->Stats       = {};
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
        if (!m_Implementation || !m_Implementation->RenderGraph
            || !m_Implementation->GeometryPipeline || !m_Implementation->ShadowPipeline)
        {
            return;
        }

        m_Implementation->RenderGraph->Execute();
    }

    void SceneRenderer::OnResize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0) return;
        if (m_Width == width && m_Height == height) return;

        Renderer::WaitIdle();

        m_Width  = width;
        m_Height = height;

        if (m_Implementation && m_Implementation->RenderGraph)
        {
            m_Implementation->RenderGraph->OnResize(width, height);
        }
    }

    std::shared_ptr<Texture> SceneRenderer::GetFinalOutput() const
    {
        if (!m_Implementation || !m_Implementation->RenderGraph)
        {
            return nullptr;
        }

        return m_Implementation->RenderGraph->GetTexture(m_Implementation->AlbedoHandle);
    }

    const SceneRendererStats& SceneRenderer::GetStats() const
    {
        return m_Implementation->Stats;
    }
}
