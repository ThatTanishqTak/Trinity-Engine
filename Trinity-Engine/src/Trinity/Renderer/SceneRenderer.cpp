#include "Trinity/Renderer/SceneRenderer.h"

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/Sampler.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"
#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Renderer/Vulkan/RenderGraph/VulkanRenderGraph.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanSampler.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Utilities/Log.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <cstring>
#include <memory>
#include <unordered_map>

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

        VkDevice Device = VK_NULL_HANDLE;
        VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout GeometryDescriptorLayout = VK_NULL_HANDLE;
        std::shared_ptr<Sampler> AlbedoSampler;
        std::shared_ptr<Texture> WhiteFallbackTexture;
        VkDescriptorSet WhiteFallbackSet = VK_NULL_HANDLE;
        std::unordered_map<Texture*, VkDescriptorSet> DescriptorSetCache;
    };

    SceneRenderer::SceneRenderer() = default;
    SceneRenderer::~SceneRenderer() = default;

    void SceneRenderer::Initialize(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;
        m_Implementation = std::make_unique<Implementation>();

        auto* a_VulkanAPI = dynamic_cast<VulkanRendererAPI*>(&Renderer::GetAPI());
        if (!a_VulkanAPI)
        {
            TR_CORE_ERROR("SceneRenderer requires Vulkan backend");
            return;
        }

        m_Implementation->RenderGraph = std::make_unique<VulkanRenderGraph>(*a_VulkanAPI);

        constexpr uint32_t l_ShadowMapSize = 2048;

        RenderGraphTextureDescription l_AlbedoDescription{};
        l_AlbedoDescription.Format = TextureFormat::RGBA8;
        l_AlbedoDescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_AlbedoDescription.DebugName = "GeometryBuffer-Albedo";
        m_Implementation->AlbedoHandle = m_Implementation->RenderGraph->DeclareTexture(l_AlbedoDescription);

        RenderGraphTextureDescription l_NormalDescription{};
        l_NormalDescription.Format = TextureFormat::RGBA16F;
        l_NormalDescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_NormalDescription.DebugName = "GeometryBuffer-Normal";
        m_Implementation->NormalHandle = m_Implementation->RenderGraph->DeclareTexture(l_NormalDescription);

        RenderGraphTextureDescription l_MRADescription{};
        l_MRADescription.Format = TextureFormat::RGBA8;
        l_MRADescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_MRADescription.DebugName = "GeometryBuffer-MRA";
        m_Implementation->MRAHandle = m_Implementation->RenderGraph->DeclareTexture(l_MRADescription);

        RenderGraphTextureDescription l_DepthDescription{};
        l_DepthDescription.Format = TextureFormat::Depth32F;
        l_DepthDescription.Usage = TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
        l_DepthDescription.DebugName = "GeometryBuffer-Depth";
        m_Implementation->DepthHandle = m_Implementation->RenderGraph->DeclareTexture(l_DepthDescription);

        RenderGraphTextureDescription l_ShadowDescription{};
        l_ShadowDescription.Width = l_ShadowMapSize;
        l_ShadowDescription.Height = l_ShadowMapSize;
        l_ShadowDescription.Format = TextureFormat::Depth32F;
        l_ShadowDescription.Usage = TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
        l_ShadowDescription.DebugName = "ShadowMap";
        m_Implementation->ShadowHandle = m_Implementation->RenderGraph->DeclareTexture(l_ShadowDescription);

        const auto a_AlbedoHandle = m_Implementation->AlbedoHandle;
        const auto a_NormalHandle = m_Implementation->NormalHandle;
        const auto a_MRAHandle = m_Implementation->MRAHandle;
        const auto a_DepthHandle = m_Implementation->DepthHandle;
        const auto a_ShadowHandle = m_Implementation->ShadowHandle;

        m_Implementation->RenderGraph->AddPass("Shadow").Write(a_ShadowHandle).SetExecuteCallback([this, a_VulkanAPI, a_ShadowHandle](RenderGraphContext& context)
        {
            VkCommandBuffer l_CommandBuffer = a_VulkanAPI->GetCurrentCommandBuffer();

            auto a_ShadowTexture = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_ShadowHandle));
            if (!a_ShadowTexture)
            {
                return;
            }

            VkRenderingAttachmentInfo l_DepthAttachment{};
            l_DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            l_DepthAttachment.imageView = a_ShadowTexture->GetImageView();
            l_DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            l_DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            l_DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            l_DepthAttachment.clearValue.depthStencil = { 1.0f, 0 };

            constexpr uint32_t l_Size = 2048;

            VkRenderingInfo l_RenderingInfo{};
            l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            l_RenderingInfo.renderArea = { { 0, 0 }, { l_Size, l_Size } };
            l_RenderingInfo.layerCount = 1;
            l_RenderingInfo.pDepthAttachment = &l_DepthAttachment;
            vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);

            VkViewport l_Viewport{};
            l_Viewport.width = static_cast<float>(l_Size);
            l_Viewport.height = static_cast<float>(l_Size);
            l_Viewport.minDepth = 0.0f;
            l_Viewport.maxDepth = 1.0f;
            vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);

            VkRect2D l_Scissor{};
            l_Scissor.extent = { l_Size, l_Size };
            vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);

            if (!m_DrawList.empty() && m_Implementation->ShadowPipeline)
            {
                auto* a_VulkanPipeline = dynamic_cast<VulkanPipeline*>(m_Implementation->ShadowPipeline.get());
                vkCmdBindPipeline(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, a_VulkanPipeline->GetPipeline());

                glm::vec3 l_LightDirection = glm::normalize(glm::vec3(m_SceneData.SunLight.Direction.x, m_SceneData.SunLight.Direction.y, m_SceneData.SunLight.Direction.z));
                const glm::vec3 l_Up = (glm::abs(glm::dot(l_LightDirection, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

                glm::mat4 l_LightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
                l_LightProjection[1][1] *= -1.0f;

                const glm::mat4 l_LightView = glm::lookAt(-l_LightDirection * 30.0f, glm::vec3(0.0f), l_Up);
                const glm::mat4 l_LightSpaceMatrix = l_LightProjection * l_LightView;

                for (const auto& it_Command : m_DrawList)
                {
                    if (!it_Command.MeshRef)
                    {
                        continue;
                    }

                    auto* a_VertexBuffer = dynamic_cast<VulkanBuffer*>(it_Command.MeshRef->GetVertexBuffer().get());
                    auto* a_IndexBuffer = dynamic_cast<VulkanBuffer*>(it_Command.MeshRef->GetIndexBuffer().get());
                    if (!a_VertexBuffer || !a_IndexBuffer) continue;

                    VkBuffer l_Buffers[] = { a_VertexBuffer->GetBuffer() };
                    VkDeviceSize l_Offsets[] = { 0 };
                    vkCmdBindVertexBuffers(l_CommandBuffer, 0, 1, l_Buffers, l_Offsets);
                    vkCmdBindIndexBuffer(l_CommandBuffer, a_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

                    struct
                    {
                        float LightSpaceMatrix[16];
                        float Model[16];
                    } l_Push{};

                    std::memcpy(l_Push.LightSpaceMatrix, glm::value_ptr(l_LightSpaceMatrix), 64);
                    std::memcpy(l_Push.Model, it_Command.Transform, 64);
                    vkCmdPushConstants(l_CommandBuffer, a_VulkanPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 128, &l_Push);

                    vkCmdDrawIndexed(l_CommandBuffer, it_Command.MeshRef->GetIndexCount(), 1, 0, 0, 0);
                }
            }

            vkCmdEndRendering(l_CommandBuffer);
        });

        m_Implementation->RenderGraph->AddPass("Geometry").Read(a_ShadowHandle).Write(a_AlbedoHandle).Write(a_NormalHandle).Write(a_MRAHandle).Write(a_DepthHandle).SetExecuteCallback([this, a_VulkanAPI, a_AlbedoHandle, a_NormalHandle, a_MRAHandle, a_DepthHandle](RenderGraphContext& context)
        {
            VkCommandBuffer l_CommandBuffer = a_VulkanAPI->GetCurrentCommandBuffer();

            auto l_Albedo = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_AlbedoHandle));
            auto l_Normal = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_NormalHandle));
            auto l_MRA = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_MRAHandle));
            auto l_Depth = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_DepthHandle));

            if (!l_Albedo || !l_Normal || !l_MRA || !l_Depth)
            {
                return;
            }

            auto a_MakeColorAttachment = [](VkImageView view) -> VkRenderingAttachmentInfo
            {
                VkRenderingAttachmentInfo l_Info{};
                l_Info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                l_Info.imageView = view;
                l_Info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                l_Info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                l_Info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                l_Info.clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

                return l_Info;
            };

            std::array<VkRenderingAttachmentInfo, 3> l_ColorAttachments = {
                a_MakeColorAttachment(l_Albedo->GetImageView()),
                a_MakeColorAttachment(l_Normal->GetImageView()),
                a_MakeColorAttachment(l_MRA->GetImageView())
            };

            VkRenderingAttachmentInfo l_DepthAttachment{};
            l_DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            l_DepthAttachment.imageView = l_Depth->GetImageView();
            l_DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            l_DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            l_DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            l_DepthAttachment.clearValue.depthStencil = { 1.0f, 0 };

            VkRenderingInfo l_RenderingInfo{};
            l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            l_RenderingInfo.renderArea = { { 0, 0 }, { m_Width, m_Height } };
            l_RenderingInfo.layerCount = 1;
            l_RenderingInfo.colorAttachmentCount = static_cast<uint32_t>(l_ColorAttachments.size());
            l_RenderingInfo.pColorAttachments = l_ColorAttachments.data();
            l_RenderingInfo.pDepthAttachment = &l_DepthAttachment;
            vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);

            VkViewport l_Viewport{};
            l_Viewport.width = static_cast<float>(m_Width);
            l_Viewport.height = static_cast<float>(m_Height);
            l_Viewport.minDepth = 0.0f;
            l_Viewport.maxDepth = 1.0f;
            vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);

            VkRect2D l_Scissor{};
            l_Scissor.extent = { m_Width, m_Height };
            vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);

            if (!m_DrawList.empty())
            {
                auto* a_VulkanPipeline = dynamic_cast<VulkanPipeline*>(m_Implementation->GeometryPipeline.get());
                auto* a_VkSampler = dynamic_cast<VulkanSampler*>(m_Implementation->AlbedoSampler.get());
                vkCmdBindPipeline(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, a_VulkanPipeline->GetPipeline());

                const glm::mat4 l_VP = m_Camera.GetViewProjectionMatrix();

                auto a_GetOrCreateSet = [&](Texture* texture) -> VkDescriptorSet
                {
                    auto it = m_Implementation->DescriptorSetCache.find(texture);
                    if (it != m_Implementation->DescriptorSetCache.end())
                        return it->second;

                    auto* a_VkTex = dynamic_cast<VulkanTexture*>(texture);
                    if (!a_VkTex || !a_VkSampler || m_Implementation->GeometryDescriptorLayout == VK_NULL_HANDLE)
                        return m_Implementation->WhiteFallbackSet;

                    VkDescriptorSetAllocateInfo l_AI{};
                    l_AI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    l_AI.descriptorPool = m_Implementation->DescriptorPool;
                    l_AI.descriptorSetCount = 1;
                    l_AI.pSetLayouts = &m_Implementation->GeometryDescriptorLayout;

                    VkDescriptorSet l_NewSet = VK_NULL_HANDLE;
                    if (vkAllocateDescriptorSets(m_Implementation->Device, &l_AI, &l_NewSet) != VK_SUCCESS)
                        return m_Implementation->WhiteFallbackSet;

                    VkDescriptorImageInfo l_Img{};
                    l_Img.sampler = a_VkSampler->GetSampler();
                    l_Img.imageView = a_VkTex->GetImageView();
                    l_Img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    VkWriteDescriptorSet l_Wr{};
                    l_Wr.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    l_Wr.dstSet = l_NewSet;
                    l_Wr.dstBinding = 0;
                    l_Wr.descriptorCount = 1;
                    l_Wr.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    l_Wr.pImageInfo = &l_Img;
                    vkUpdateDescriptorSets(m_Implementation->Device, 1, &l_Wr, 0, nullptr);

                    m_Implementation->DescriptorSetCache[texture] = l_NewSet;
                    return l_NewSet;
                };

                for (const auto& it_Command : m_DrawList)
                {
                    if (!it_Command.MeshRef) continue;

                    auto* a_VertexBuffer = dynamic_cast<VulkanBuffer*>(it_Command.MeshRef->GetVertexBuffer().get());
                    auto* a_IndexBuffer = dynamic_cast<VulkanBuffer*>(it_Command.MeshRef->GetIndexBuffer().get());
                    if (!a_VertexBuffer || !a_IndexBuffer) continue;

                    VkBuffer l_Buffers[] = { a_VertexBuffer->GetBuffer() };
                    VkDeviceSize l_Offsets[] = { 0 };
                    vkCmdBindVertexBuffers(l_CommandBuffer, 0, 1, l_Buffers, l_Offsets);
                    vkCmdBindIndexBuffer(l_CommandBuffer, a_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

                    Texture* l_AlbedoTex = it_Command.AlbedoTexture
                        ? it_Command.AlbedoTexture.get()
                        : m_Implementation->WhiteFallbackTexture.get();
                    VkDescriptorSet l_DescSet = a_GetOrCreateSet(l_AlbedoTex);
                    if (l_DescSet != VK_NULL_HANDLE)
                        vkCmdBindDescriptorSets(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, a_VulkanPipeline->GetLayout(), 0, 1, &l_DescSet, 0, nullptr);

                    struct
                    {
                        float Model[16]; float VP[16];
                    } l_Push;
                    std::memcpy(l_Push.Model, it_Command.Transform, 64);
                    std::memcpy(l_Push.VP, glm::value_ptr(l_VP), 64);
                    vkCmdPushConstants(l_CommandBuffer, a_VulkanPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 128, &l_Push);

                    vkCmdDrawIndexed(l_CommandBuffer, it_Command.MeshRef->GetIndexCount(), 1, 0, 0, 0);

                    m_Implementation->Stats.DrawCalls++;
                    m_Implementation->Stats.IndexCount += it_Command.MeshRef->GetIndexCount();
                    m_Implementation->Stats.VertexCount += it_Command.MeshRef->GetVertexCount();
                }
            }

            vkCmdEndRendering(l_CommandBuffer);
        });

        m_Implementation->RenderGraph->AddPass("Output").Read(a_AlbedoHandle);

        ShaderSpecification l_ShadowShaderSpec{};
        l_ShadowShaderSpec.Modules.push_back({ ShaderStage::Vertex,   "shadow_pass.vert.spv" });
        l_ShadowShaderSpec.Modules.push_back({ ShaderStage::Fragment, "shadow_pass.frag.spv" });
        l_ShadowShaderSpec.DebugName = "ShadowShader";
        auto l_ShadowShader = Renderer::GetAPI().CreateShader(l_ShadowShaderSpec);

        PipelineSpecification l_ShadowPipelineSpec{};
        l_ShadowPipelineSpec.PipelineShader = l_ShadowShader;
        l_ShadowPipelineSpec.VertexAttributes = { { 0, 0, VertexAttributeFormat::Float3, 0 } };
        l_ShadowPipelineSpec.VertexStride = sizeof(Vertex);
        l_ShadowPipelineSpec.PushConstants = { { ShaderStage::Vertex, 0, 128 } };
        l_ShadowPipelineSpec.DepthAttachmentFormat = TextureFormat::Depth32F;
        l_ShadowPipelineSpec.DepthTest = true;
        l_ShadowPipelineSpec.DepthWrite = true;
        l_ShadowPipelineSpec.DepthOp = DepthCompareOp::Less;
        l_ShadowPipelineSpec.CullingMode = CullMode::Back;
        l_ShadowPipelineSpec.DepthBias = true;
        l_ShadowPipelineSpec.DepthBiasConstantFactor = 1.25f;
        l_ShadowPipelineSpec.DepthBiasSlopeFactor = 1.75f;
        l_ShadowPipelineSpec.DebugName = "ShadowPipeline";
        m_Implementation->ShadowPipeline = Renderer::GetAPI().CreatePipeline(l_ShadowPipelineSpec);

        ShaderSpecification l_ShaderSpec{};
        l_ShaderSpec.Modules.push_back({ ShaderStage::Vertex,   "geometry_pass.vert.spv" });
        l_ShaderSpec.Modules.push_back({ ShaderStage::Fragment, "geometry_pass.frag.spv" });
        l_ShaderSpec.DebugName = "GeometryShader";
        auto l_Shader = Renderer::GetAPI().CreateShader(l_ShaderSpec);

        PipelineSpecification l_PipelineSpec{};
        l_PipelineSpec.PipelineShader = l_Shader;
        l_PipelineSpec.VertexAttributes = { { 0, 0, VertexAttributeFormat::Float3,  0 }, { 1, 0, VertexAttributeFormat::Float3, 12 }, { 2, 0, VertexAttributeFormat::Float2, 24 } };
        l_PipelineSpec.VertexStride = sizeof(Vertex);
        l_PipelineSpec.PushConstants = { { ShaderStage::Vertex, 0, 128 } };
        l_PipelineSpec.ColorAttachmentFormats = { TextureFormat::RGBA8, TextureFormat::RGBA16F, TextureFormat::RGBA8 };
        l_PipelineSpec.DepthAttachmentFormat = TextureFormat::Depth32F;
        l_PipelineSpec.DepthTest = true;
        l_PipelineSpec.DepthWrite = true;
        l_PipelineSpec.DepthOp = DepthCompareOp::Less;
        l_PipelineSpec.CullingMode = CullMode::Back;
        l_PipelineSpec.DescriptorBindings = { { 0, DescriptorType::CombinedImageSampler, ShaderStage::Fragment, 1 } };
        l_PipelineSpec.DebugName = "GeometryPipeline";
        m_Implementation->GeometryPipeline = Renderer::GetAPI().CreatePipeline(l_PipelineSpec);

        m_Implementation->Device = a_VulkanAPI->GetDevice().GetDevice();

        auto* a_VkGeomPipeline = dynamic_cast<VulkanPipeline*>(m_Implementation->GeometryPipeline.get());
        m_Implementation->GeometryDescriptorLayout = a_VkGeomPipeline->GetDescriptorSetLayout();

        VkDescriptorPoolSize l_PoolSize{};
        l_PoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_PoolSize.descriptorCount = 256;

        VkDescriptorPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_PoolInfo.maxSets = 256;
        l_PoolInfo.poolSizeCount = 1;
        l_PoolInfo.pPoolSizes = &l_PoolSize;
        VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Implementation->Device, &l_PoolInfo, nullptr, &m_Implementation->DescriptorPool), "Failed vkCreateDescriptorPool");

        SamplerSpecification l_SamplerSpec{};
        l_SamplerSpec.MinFilter = SamplerFilter::Linear;
        l_SamplerSpec.MagFilter = SamplerFilter::Linear;
        l_SamplerSpec.MipmapMode = SamplerMipmapMode::Linear;
        l_SamplerSpec.AddressModeU = SamplerAddressMode::Repeat;
        l_SamplerSpec.AddressModeV = SamplerAddressMode::Repeat;
        l_SamplerSpec.AddressModeW = SamplerAddressMode::Repeat;
        l_SamplerSpec.DebugName = "AlbedoSampler";
        m_Implementation->AlbedoSampler = Renderer::GetAPI().CreateSampler(l_SamplerSpec);

        const uint8_t l_White[4] = { 255, 255, 255, 255 };
        m_Implementation->WhiteFallbackTexture = Renderer::GetAPI().CreateTextureFromData(l_White, 1, 1);

        auto* a_VkSampler = dynamic_cast<VulkanSampler*>(m_Implementation->AlbedoSampler.get());
        auto* a_VkWhiteTex = dynamic_cast<VulkanTexture*>(m_Implementation->WhiteFallbackTexture.get());

        if (a_VkSampler && a_VkWhiteTex && m_Implementation->GeometryDescriptorLayout != VK_NULL_HANDLE)
        {
            VkDescriptorSetAllocateInfo l_AllocInfo{};
            l_AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            l_AllocInfo.descriptorPool = m_Implementation->DescriptorPool;
            l_AllocInfo.descriptorSetCount = 1;
            l_AllocInfo.pSetLayouts = &m_Implementation->GeometryDescriptorLayout;
            VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Implementation->Device, &l_AllocInfo, &m_Implementation->WhiteFallbackSet), "Failed vkAllocateDescriptorSets");

            VkDescriptorImageInfo l_ImageInfo{};
            l_ImageInfo.sampler = a_VkSampler->GetSampler();
            l_ImageInfo.imageView = a_VkWhiteTex->GetImageView();
            l_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet l_Write{};
            l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            l_Write.dstSet = m_Implementation->WhiteFallbackSet;
            l_Write.dstBinding = 0;
            l_Write.descriptorCount = 1;
            l_Write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            l_Write.pImageInfo = &l_ImageInfo;
            vkUpdateDescriptorSets(m_Implementation->Device, 1, &l_Write, 0, nullptr);

            m_Implementation->DescriptorSetCache[m_Implementation->WhiteFallbackTexture.get()] = m_Implementation->WhiteFallbackSet;
        }
    }

    void SceneRenderer::Shutdown()
    {
        if (m_Implementation && m_Implementation->DescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_Implementation->Device, m_Implementation->DescriptorPool, nullptr);

            m_Implementation->DescriptorPool = VK_NULL_HANDLE;
            m_Implementation->DescriptorSetCache.clear();
            m_Implementation->WhiteFallbackSet = VK_NULL_HANDLE;
        }

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
        if (!m_Implementation || !m_Implementation->RenderGraph || !m_Implementation->GeometryPipeline || !m_Implementation->ShadowPipeline)
        {
            return;
        }

        m_Implementation->RenderGraph->Execute();
    }

    void SceneRenderer::OnResize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0 || ((m_Width == width) && (m_Height == height)))
        {
            return;
        }

        Renderer::WaitIdle();

        m_Width = width;
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