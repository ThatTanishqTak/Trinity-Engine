#include "Trinity/Renderer/Vulkan/VulkanSceneRenderer.h"

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/Sampler.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Renderer/RenderGraph/RenderGraph.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"
#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanSampler.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Utilities/Log.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace Trinity
{
    struct LightingUniformData
    {
        float InvViewProjection[16];
        float LightSpaceMatrix[16];
        float LightDirection[4];
        float LightColor[4];
        float CameraPosition[4];
    };

    struct GpuLight
    {
        float PositionAndRange[4];
        float DirectionAndInnerCos[4];
        float ColorAndIntensity[4];
        float Params0[4];
        float Params1[4];
    };

    static constexpr uint32_t s_MaxSupportedSceneLights = 256;
    static constexpr uint32_t s_GpuLightTypePoint = 1;
    static constexpr uint32_t s_GpuLightTypeSpot = 2;

    template <typename TLight>
    static GpuLight BuildGpuLightFromData(const TLight& light)
    {
        GpuLight l_Result{};

        if constexpr (requires { light.Position; })
        {
            l_Result.PositionAndRange[0] = light.Position.x;
            l_Result.PositionAndRange[1] = light.Position.y;
            l_Result.PositionAndRange[2] = light.Position.z;
        }

        if constexpr (requires { light.Range; })
        {
            l_Result.PositionAndRange[3] = light.Range;
        }
        else
        {
            l_Result.PositionAndRange[3] = 10.0f;
        }

        if constexpr (requires { light.Direction; })
        {
            l_Result.DirectionAndInnerCos[0] = light.Direction.x;
            l_Result.DirectionAndInnerCos[1] = light.Direction.y;
            l_Result.DirectionAndInnerCos[2] = light.Direction.z;
        }

        if constexpr (requires { light.InnerCos; })
        {
            l_Result.DirectionAndInnerCos[3] = light.InnerCos;
        }
        else if constexpr (requires { light.InnerAngle; })
        {
            l_Result.DirectionAndInnerCos[3] = std::cos(light.InnerAngle);
        }
        else
        {
            l_Result.DirectionAndInnerCos[3] = 1.0f;
        }

        if constexpr (requires { light.Color; })
        {
            l_Result.ColorAndIntensity[0] = light.Color.r;
            l_Result.ColorAndIntensity[1] = light.Color.g;
            l_Result.ColorAndIntensity[2] = light.Color.b;
        }
        else
        {
            l_Result.ColorAndIntensity[0] = 1.0f;
            l_Result.ColorAndIntensity[1] = 1.0f;
            l_Result.ColorAndIntensity[2] = 1.0f;
        }

        if constexpr (requires { light.Intensity; })
        {
            l_Result.ColorAndIntensity[3] = light.Intensity;
        }
        else
        {
            l_Result.ColorAndIntensity[3] = 1.0f;
        }

        if constexpr (requires { light.OuterCos; })
        {
            l_Result.Params0[0] = light.OuterCos;
        }
        else if constexpr (requires { light.OuterAngle; })
        {
            l_Result.Params0[0] = std::cos(light.OuterAngle);
        }

        if constexpr (requires { light.Type; })
        {
            l_Result.Params1[3] = static_cast<float>(light.Type);
        }
        else if constexpr (requires { light.Direction; })
        {
            l_Result.Params1[3] = static_cast<float>(s_GpuLightTypeSpot);
        }
        else
        {
            l_Result.Params1[3] = static_cast<float>(s_GpuLightTypePoint);
        }

        return l_Result;
    }

    template <typename TSceneData>
    static uint32_t FillGpuLightsFromSceneData(const TSceneData& sceneData, std::vector<GpuLight>& outLights)
    {
        outLights.clear();

        if constexpr (requires { sceneData.Lights; })
        {
            const size_t l_Count = std::min(static_cast<size_t>(s_MaxSupportedSceneLights), sceneData.Lights.size());
            outLights.reserve(l_Count);

            for (size_t i = 0; i < l_Count; i++)
            {
                const auto& it_Light = sceneData.Lights[i];
                if constexpr (std::is_same_v<std::remove_cvref_t<decltype(it_Light)>, GpuLight>)
                {
                    outLights.push_back(it_Light);
                }
                else
                {
                    outLights.push_back(BuildGpuLightFromData(it_Light));
                }
            }

            return static_cast<uint32_t>(outLights.size());
        }

        return 0;
    }

    struct VulkanSceneRenderer::Implementation
    {
        SceneRendererStats Stats;
        bool SceneActive = false;
        std::unique_ptr<RenderGraph> RenderGraph;

        std::shared_ptr<Pipeline> GeometryPipeline;
        std::shared_ptr<Pipeline> ShadowPipeline;
        std::shared_ptr<Pipeline> LightingPipeline;

        RenderGraphResourceHandle AlbedoHandle;
        RenderGraphResourceHandle NormalHandle;
        RenderGraphResourceHandle MRAHandle;
        RenderGraphResourceHandle DepthHandle;
        RenderGraphResourceHandle ShadowHandle;
        RenderGraphResourceHandle LitOutputHandle;

        VkDevice Device = VK_NULL_HANDLE;
        VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout GeometryDescriptorLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout LightingDescriptorLayout = VK_NULL_HANDLE;
        VkDescriptorSet WhiteFallbackSet = VK_NULL_HANDLE;

        std::shared_ptr<Sampler> AlbedoSampler;
        std::shared_ptr<Sampler> GBufferSampler;
        std::shared_ptr<Texture> WhiteFallbackTexture;

        std::unordered_map<Texture*, VkDescriptorSet> DescriptorSetCache;

        std::vector<std::shared_ptr<Buffer>> LightingUniformBuffers;
        std::vector<VkDescriptorSet> LightingDescriptorSets;
        std::shared_ptr<Buffer> LightStorageBuffer;
    };

    VulkanSceneRenderer::VulkanSceneRenderer() = default;
    VulkanSceneRenderer::~VulkanSceneRenderer() = default;

    bool VulkanSceneRenderer::BindMeshBuffers(VkCommandBuffer commandBuffer, const MeshDrawCommand& command) const
    {
        if (!command.MeshRef)
        {
            return false;
        }

        auto* a_VertexBuffer = dynamic_cast<VulkanBuffer*>(command.MeshRef->GetVertexBuffer().get());
        auto* a_IndexBuffer = dynamic_cast<VulkanBuffer*>(command.MeshRef->GetIndexBuffer().get());
        if (!a_VertexBuffer || !a_IndexBuffer)
        {
            return false;
        }

        VkBuffer l_Buffers[] = { a_VertexBuffer->GetBuffer() };
        VkDeviceSize l_Offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, l_Buffers, l_Offsets);
        vkCmdBindIndexBuffer(commandBuffer, a_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        return true;
    }

    void VulkanSceneRenderer::DrawMeshCommand(VkCommandBuffer commandBuffer, const MeshDrawCommand& command) const
    {
        vkCmdDrawIndexed(commandBuffer, command.MeshRef->GetIndexCount(), 1, 0, 0, 0);
    }

    VkDescriptorSet VulkanSceneRenderer::GetOrCreateGeometryMaterialSet(Texture* texture)
    {
        auto a_Index = m_Implementation->DescriptorSetCache.find(texture);
        if (a_Index != m_Implementation->DescriptorSetCache.end())
        {
            return a_Index->second;
        }

        auto* a_VkSampler = dynamic_cast<VulkanSampler*>(m_Implementation->AlbedoSampler.get());
        auto* a_VkTexture = dynamic_cast<VulkanTexture*>(texture);
        if (!a_VkTexture || !a_VkSampler || m_Implementation->GeometryDescriptorLayout == VK_NULL_HANDLE)
        {
            return m_Implementation->WhiteFallbackSet;
        }

        VkDescriptorSetAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_AllocateInfo.descriptorPool = m_Implementation->DescriptorPool;
        l_AllocateInfo.descriptorSetCount = 1;
        l_AllocateInfo.pSetLayouts = &m_Implementation->GeometryDescriptorLayout;

        VkDescriptorSet l_NewDescriptorSet = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(m_Implementation->Device, &l_AllocateInfo, &l_NewDescriptorSet) != VK_SUCCESS)
        {
            return m_Implementation->WhiteFallbackSet;
        }

        VkDescriptorImageInfo l_ImageInfo{};
        l_ImageInfo.sampler = a_VkSampler->GetSampler();
        l_ImageInfo.imageView = a_VkTexture->GetImageView();
        l_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet l_WriteDescriptorSet{};
        l_WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_WriteDescriptorSet.dstSet = l_NewDescriptorSet;
        l_WriteDescriptorSet.dstBinding = 0;
        l_WriteDescriptorSet.descriptorCount = 1;
        l_WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_WriteDescriptorSet.pImageInfo = &l_ImageInfo;
        vkUpdateDescriptorSets(m_Implementation->Device, 1, &l_WriteDescriptorSet, 0, nullptr);

        m_Implementation->DescriptorSetCache[texture] = l_NewDescriptorSet;
        return l_NewDescriptorSet;
    }

    void VulkanSceneRenderer::BindGeometryMaterialSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Texture* texture)
    {
        VkDescriptorSet l_DescriptorSet = GetOrCreateGeometryMaterialSet(texture);
        if (l_DescriptorSet != VK_NULL_HANDLE)
        {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &l_DescriptorSet, 0, nullptr);
        }
    }

    void VulkanSceneRenderer::UpdateLightingDescriptorSet(uint32_t frameIndex, VulkanSampler* sampler, VulkanTexture* albedo, VulkanTexture* normal, VulkanTexture* mra, VulkanTexture* depth, VulkanTexture* shadow)
    {
        auto* a_VkUBO = dynamic_cast<VulkanBuffer*>(m_Implementation->LightingUniformBuffers[frameIndex].get());
        auto* a_VkLightBuffer = dynamic_cast<VulkanBuffer*>(m_Implementation->LightStorageBuffer.get());
        if (!sampler || !a_VkUBO || !a_VkLightBuffer)
        {
            return;
        }

        VkDescriptorSet l_Set = m_Implementation->LightingDescriptorSets[frameIndex];

        std::array<VkDescriptorImageInfo, 5> l_ImageInfos{};
        l_ImageInfos[0] = { sampler->GetSampler(), albedo->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        l_ImageInfos[1] = { sampler->GetSampler(), normal->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        l_ImageInfos[2] = { sampler->GetSampler(), mra->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        l_ImageInfos[3] = { sampler->GetSampler(), depth->GetImageView(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
        l_ImageInfos[4] = { sampler->GetSampler(), shadow->GetImageView(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };

        VkDescriptorBufferInfo l_UBOInfo{};
        l_UBOInfo.buffer = a_VkUBO->GetBuffer();
        l_UBOInfo.offset = 0;
        l_UBOInfo.range = sizeof(LightingUniformData);

        VkDescriptorBufferInfo l_SSBOInfo{};
        l_SSBOInfo.buffer = a_VkLightBuffer->GetBuffer();
        l_SSBOInfo.offset = 0;
        l_SSBOInfo.range = m_Implementation->LightStorageBuffer->GetSize();

        if (l_SSBOInfo.buffer == VK_NULL_HANDLE || l_SSBOInfo.range == 0)
        {
            return;
        }

        std::array<VkWriteDescriptorSet, 7> l_WriteDescriptorSets{};
        for (uint32_t i = 0; i < 5; i++)
        {
            l_WriteDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            l_WriteDescriptorSets[i].dstSet = l_Set;
            l_WriteDescriptorSets[i].dstBinding = i;
            l_WriteDescriptorSets[i].descriptorCount = 1;
            l_WriteDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            l_WriteDescriptorSets[i].pImageInfo = &l_ImageInfos[i];
        }

        l_WriteDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_WriteDescriptorSets[5].dstSet = l_Set;
        l_WriteDescriptorSets[5].dstBinding = 5;
        l_WriteDescriptorSets[5].descriptorCount = 1;
        l_WriteDescriptorSets[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_WriteDescriptorSets[5].pBufferInfo = &l_UBOInfo;

        l_WriteDescriptorSets[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_WriteDescriptorSets[6].dstSet = l_Set;
        l_WriteDescriptorSets[6].dstBinding = 6;
        l_WriteDescriptorSets[6].descriptorCount = 1;
        l_WriteDescriptorSets[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        l_WriteDescriptorSets[6].pBufferInfo = &l_SSBOInfo;
        vkUpdateDescriptorSets(m_Implementation->Device, 7, l_WriteDescriptorSets.data(), 0, nullptr);
    }

    void VulkanSceneRenderer::SetupFallbackTextureAndSampler()
    {
        const uint8_t l_White[4] = { 255, 255, 255, 255 };
        m_Implementation->WhiteFallbackTexture = Renderer::GetAPI().CreateTextureFromData(l_White, 1, 1);

        auto* a_VkSampler = dynamic_cast<VulkanSampler*>(m_Implementation->AlbedoSampler.get());
        auto* a_VkWhiteTexture = dynamic_cast<VulkanTexture*>(m_Implementation->WhiteFallbackTexture.get());
        if (!a_VkSampler || !a_VkWhiteTexture || m_Implementation->GeometryDescriptorLayout == VK_NULL_HANDLE)
        {
            return;
        }

        VkDescriptorSetAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_AllocateInfo.descriptorPool = m_Implementation->DescriptorPool;
        l_AllocateInfo.descriptorSetCount = 1;
        l_AllocateInfo.pSetLayouts = &m_Implementation->GeometryDescriptorLayout;
        VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Implementation->Device, &l_AllocateInfo, &m_Implementation->WhiteFallbackSet), "Failed vkAllocateDescriptorSets");

        VkDescriptorImageInfo l_ImageInfo{};
        l_ImageInfo.sampler = a_VkSampler->GetSampler();
        l_ImageInfo.imageView = a_VkWhiteTexture->GetImageView();
        l_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet l_WriteDescriptorSet{};
        l_WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_WriteDescriptorSet.dstSet = m_Implementation->WhiteFallbackSet;
        l_WriteDescriptorSet.dstBinding = 0;
        l_WriteDescriptorSet.descriptorCount = 1;
        l_WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_WriteDescriptorSet.pImageInfo = &l_ImageInfo;
        vkUpdateDescriptorSets(m_Implementation->Device, 1, &l_WriteDescriptorSet, 0, nullptr);

        m_Implementation->DescriptorSetCache[m_Implementation->WhiteFallbackTexture.get()] = m_Implementation->WhiteFallbackSet;
    }

    void VulkanSceneRenderer::Initialize(uint32_t width, uint32_t height)
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

        m_Implementation->RenderGraph = a_VulkanAPI->CreateRenderGraph();

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

        RenderGraphTextureDescription l_LitOutputDescription{};
        l_LitOutputDescription.Format = TextureFormat::RGBA8;
        l_LitOutputDescription.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
        l_LitOutputDescription.DebugName = "LitOutput";
        m_Implementation->LitOutputHandle = m_Implementation->RenderGraph->DeclareTexture(l_LitOutputDescription);

        const auto a_AlbedoHandle = m_Implementation->AlbedoHandle;
        const auto a_NormalHandle = m_Implementation->NormalHandle;
        const auto a_MRAHandle = m_Implementation->MRAHandle;
        const auto a_DepthHandle = m_Implementation->DepthHandle;
        const auto a_ShadowHandle = m_Implementation->ShadowHandle;
        const auto a_LitOutputHandle = m_Implementation->LitOutputHandle;

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
                    if (!BindMeshBuffers(l_CommandBuffer, it_Command))
                    {
                        continue;
                    }

                    struct
                    {
                        float LightSpaceMatrix[16];
                        float Model[16];
                    } l_Push{};

                    std::memcpy(l_Push.LightSpaceMatrix, glm::value_ptr(l_LightSpaceMatrix), 64);
                    std::memcpy(l_Push.Model, it_Command.Transform, 64);
                    vkCmdPushConstants(l_CommandBuffer, a_VulkanPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 128, &l_Push);

                    DrawMeshCommand(l_CommandBuffer, it_Command);
                }
            }

            vkCmdEndRendering(l_CommandBuffer);
        });

        m_Implementation->RenderGraph->AddPass("Geometry").Read(a_ShadowHandle).Write(a_AlbedoHandle).Write(a_NormalHandle).Write(a_MRAHandle).Write(a_DepthHandle).SetExecuteCallback([this, a_VulkanAPI, a_AlbedoHandle, a_NormalHandle, a_MRAHandle, a_DepthHandle](RenderGraphContext& context)
        {
            VkCommandBuffer l_CommandBuffer = a_VulkanAPI->GetCurrentCommandBuffer();

            auto a_Albedo = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_AlbedoHandle));
            auto a_Normal = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_NormalHandle));
            auto a_MRA = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_MRAHandle));
            auto a_Depth = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_DepthHandle));

            if (!a_Albedo || !a_Normal || !a_MRA || !a_Depth)
            {
                return;
            }

            auto a_MakeColorAttachment = [](VkImageView view) -> VkRenderingAttachmentInfo
            {
                VkRenderingAttachmentInfo l_AttachmentInfo{};
                l_AttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                l_AttachmentInfo.imageView = view;
                l_AttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                l_AttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                l_AttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                l_AttachmentInfo.clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

                return l_AttachmentInfo;
            };

            std::array<VkRenderingAttachmentInfo, 3> l_ColorAttachments = {
                a_MakeColorAttachment(a_Albedo->GetImageView()),
                a_MakeColorAttachment(a_Normal->GetImageView()),
                a_MakeColorAttachment(a_MRA->GetImageView())
            };

            VkRenderingAttachmentInfo l_DepthAttachment{};
            l_DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            l_DepthAttachment.imageView = a_Depth->GetImageView();
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
                vkCmdBindPipeline(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, a_VulkanPipeline->GetPipeline());

                const glm::mat4 l_ViewProjection = m_Camera.GetViewProjectionMatrix();

                for (const auto& it_Command : m_DrawList)
                {
                    if (!BindMeshBuffers(l_CommandBuffer, it_Command))
                    {
                        continue;
                    }

                    Texture* l_AlbedoTex = it_Command.AlbedoTexture ? it_Command.AlbedoTexture.get() : m_Implementation->WhiteFallbackTexture.get();
                    BindGeometryMaterialSet(l_CommandBuffer, a_VulkanPipeline->GetLayout(), l_AlbedoTex);

                    struct
                    {
                        float Model[16]; float VP[16];
                    } l_Push;
                    std::memcpy(l_Push.Model, it_Command.Transform, 64);
                    std::memcpy(l_Push.VP, glm::value_ptr(l_ViewProjection), 64);
                    vkCmdPushConstants(l_CommandBuffer, a_VulkanPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 128, &l_Push);

                    DrawMeshCommand(l_CommandBuffer, it_Command);

                    m_Implementation->Stats.DrawCalls++;
                    m_Implementation->Stats.IndexCount += it_Command.MeshRef->GetIndexCount();
                    m_Implementation->Stats.VertexCount += it_Command.MeshRef->GetVertexCount();
                }
            }

            vkCmdEndRendering(l_CommandBuffer);
        });

        m_Implementation->RenderGraph->AddPass("Lighting").Read(a_AlbedoHandle).Read(a_NormalHandle).Read(a_MRAHandle).Read(a_DepthHandle).Read(a_ShadowHandle).Write(a_LitOutputHandle).SetExecuteCallback([this, a_VulkanAPI, a_AlbedoHandle, a_NormalHandle, a_MRAHandle, a_DepthHandle, a_ShadowHandle, a_LitOutputHandle](RenderGraphContext& context)
        {
            VkCommandBuffer l_CommandBuffer = a_VulkanAPI->GetCurrentCommandBuffer();

            auto a_Albedo = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_AlbedoHandle));
            auto a_Normal = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_NormalHandle));
            auto a_MRA = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_MRAHandle));
            auto a_Depth = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_DepthHandle));
            auto a_Shadow = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_ShadowHandle));
            auto a_LitOutput = std::dynamic_pointer_cast<VulkanTexture>(context.GetTexture(a_LitOutputHandle));

            if (!a_Albedo || !a_Normal || !a_MRA || !a_Depth || !a_Shadow || !a_LitOutput || !m_Implementation->LightingPipeline)
            {
                return;
            }

            const uint32_t l_FrameIndex = Renderer::GetAPI().GetCurrentFrameIndex();

            const glm::mat4 l_ViewProjection = m_Camera.GetViewProjectionMatrix();
            const glm::mat4 l_InverseViewProjection = glm::inverse(l_ViewProjection);

            const glm::vec3 l_LightDirection = glm::normalize(glm::vec3(m_SceneData.SunLight.Direction));
            const glm::vec3 l_LightUp = (glm::abs(glm::dot(l_LightDirection, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
            glm::mat4 l_LightProj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
            l_LightProj[1][1] *= -1.0f;

            const glm::mat4 l_LightView = glm::lookAt(-l_LightDirection * 30.0f, glm::vec3(0.0f), l_LightUp);
            const glm::mat4 l_LightSpaceMatrix = l_LightProj * l_LightView;

            LightingUniformData l_UBOData{};
            std::memcpy(l_UBOData.InvViewProjection, glm::value_ptr(l_InverseViewProjection), 64);
            std::memcpy(l_UBOData.LightSpaceMatrix, glm::value_ptr(l_LightSpaceMatrix), 64);

            l_UBOData.LightDirection[0] = m_SceneData.SunLight.Direction.x;
            l_UBOData.LightDirection[1] = m_SceneData.SunLight.Direction.y;
            l_UBOData.LightDirection[2] = m_SceneData.SunLight.Direction.z;
            l_UBOData.LightDirection[3] = 0.0f;
            l_UBOData.LightColor[0] = m_SceneData.SunLight.Color.r;
            l_UBOData.LightColor[1] = m_SceneData.SunLight.Color.g;
            l_UBOData.LightColor[2] = m_SceneData.SunLight.Color.b;
            l_UBOData.LightColor[3] = m_SceneData.SunLight.Intensity;

            const glm::vec3 l_CamPos = m_Camera.GetPosition();
            l_UBOData.CameraPosition[0] = l_CamPos.x;
            l_UBOData.CameraPosition[1] = l_CamPos.y;
            l_UBOData.CameraPosition[2] = l_CamPos.z;

            std::vector<GpuLight> l_GpuLights;
            const uint32_t l_LightCount = FillGpuLightsFromSceneData(m_SceneData, l_GpuLights);
            l_UBOData.CameraPosition[3] = static_cast<float>(l_LightCount);

            if (m_Implementation->LightStorageBuffer && m_Implementation->LightStorageBuffer->GetSize() > 0)
            {
                if (!l_GpuLights.empty())
                {
                    m_Implementation->LightStorageBuffer->SetData(l_GpuLights.data(), static_cast<uint64_t>(l_GpuLights.size()) * sizeof(GpuLight));
                }
                else
                {
                    GpuLight l_EmptyLight{};
                    m_Implementation->LightStorageBuffer->SetData(&l_EmptyLight, sizeof(GpuLight));
                }
            }
            m_Implementation->LightingUniformBuffers[l_FrameIndex]->SetData(&l_UBOData, sizeof(LightingUniformData));

            auto* a_VulkanSampler = dynamic_cast<VulkanSampler*>(m_Implementation->GBufferSampler.get());
            VkDescriptorSet l_Set = m_Implementation->LightingDescriptorSets[l_FrameIndex];

            UpdateLightingDescriptorSet(l_FrameIndex, a_VulkanSampler, a_Albedo.get(), a_Normal.get(), a_MRA.get(), a_Depth.get(), a_Shadow.get());

            VkRenderingAttachmentInfo l_ColorAttachment{};
            l_ColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            l_ColorAttachment.imageView = a_LitOutput->GetImageView();
            l_ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            l_ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            l_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            l_ColorAttachment.clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

            VkRenderingInfo l_RenderingInfo{};
            l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            l_RenderingInfo.renderArea = { { 0, 0 }, { m_Width, m_Height } };
            l_RenderingInfo.layerCount = 1;
            l_RenderingInfo.colorAttachmentCount = 1;
            l_RenderingInfo.pColorAttachments = &l_ColorAttachment;
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

            auto* a_VkLightingPipeline = dynamic_cast<VulkanPipeline*>(m_Implementation->LightingPipeline.get());
            vkCmdBindPipeline(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, a_VkLightingPipeline->GetPipeline());
            vkCmdBindDescriptorSets(l_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, a_VkLightingPipeline->GetLayout(), 0, 1, &l_Set, 0, nullptr);
            vkCmdDraw(l_CommandBuffer, 3, 1, 0, 0);

            vkCmdEndRendering(l_CommandBuffer);
        });

        m_Implementation->RenderGraph->AddPass("Output").Read(a_LitOutputHandle);

        ShaderSpecification l_ShadowShaderSpecification{};
        l_ShadowShaderSpecification.Modules.push_back({ ShaderStage::Vertex, "shadow_pass.vert.spv" });
        l_ShadowShaderSpecification.Modules.push_back({ ShaderStage::Fragment, "shadow_pass.frag.spv" });
        l_ShadowShaderSpecification.DebugName = "ShadowShader";
        auto a_ShadowShader = Renderer::GetAPI().CreateShader(l_ShadowShaderSpecification);

        PipelineSpecification l_ShadowPipelineSpecification{};
        l_ShadowPipelineSpecification.PipelineShader = a_ShadowShader;
        l_ShadowPipelineSpecification.VertexAttributes = { { 0, 0, VertexAttributeFormat::Float3, 0 } };
        l_ShadowPipelineSpecification.VertexStride = sizeof(Vertex);
        l_ShadowPipelineSpecification.PushConstants = { { ShaderStage::Vertex, 0, 128 } };
        l_ShadowPipelineSpecification.DepthAttachmentFormat = TextureFormat::Depth32F;
        l_ShadowPipelineSpecification.DepthTest = true;
        l_ShadowPipelineSpecification.DepthWrite = true;
        l_ShadowPipelineSpecification.DepthOp = DepthCompareOp::Less;
        l_ShadowPipelineSpecification.CullingMode = CullMode::Back;
        l_ShadowPipelineSpecification.DepthBias = true;
        l_ShadowPipelineSpecification.DepthBiasConstantFactor = 1.25f;
        l_ShadowPipelineSpecification.DepthBiasSlopeFactor = 1.75f;
        l_ShadowPipelineSpecification.DebugName = "ShadowPipeline";
        m_Implementation->ShadowPipeline = Renderer::GetAPI().CreatePipeline(l_ShadowPipelineSpecification);

        ShaderSpecification l_ShaderSpecification{};
        l_ShaderSpecification.Modules.push_back({ ShaderStage::Vertex, "geometry_pass.vert.spv" });
        l_ShaderSpecification.Modules.push_back({ ShaderStage::Fragment, "geometry_pass.frag.spv" });
        l_ShaderSpecification.DebugName = "GeometryShader";
        auto a_Shader = Renderer::GetAPI().CreateShader(l_ShaderSpecification);

        PipelineSpecification l_PipelineSpecification{};
        l_PipelineSpecification.PipelineShader = a_Shader;
        l_PipelineSpecification.VertexAttributes = { { 0, 0, VertexAttributeFormat::Float3,  0 }, { 1, 0, VertexAttributeFormat::Float3, 12 }, { 2, 0, VertexAttributeFormat::Float2, 24 } };
        l_PipelineSpecification.VertexStride = sizeof(Vertex);
        l_PipelineSpecification.PushConstants = { { ShaderStage::Vertex, 0, 128 } };
        l_PipelineSpecification.ColorAttachmentFormats = { TextureFormat::RGBA8, TextureFormat::RGBA16F, TextureFormat::RGBA8 };
        l_PipelineSpecification.DepthAttachmentFormat = TextureFormat::Depth32F;
        l_PipelineSpecification.DepthTest = true;
        l_PipelineSpecification.DepthWrite = true;
        l_PipelineSpecification.DepthOp = DepthCompareOp::Less;
        l_PipelineSpecification.CullingMode = CullMode::Back;
        l_PipelineSpecification.DescriptorBindings = { { 0, DescriptorType::CombinedImageSampler, ShaderStage::Fragment, 1 } };
        l_PipelineSpecification.DebugName = "GeometryPipeline";
        m_Implementation->GeometryPipeline = Renderer::GetAPI().CreatePipeline(l_PipelineSpecification);

        ShaderSpecification l_LightingShaderSpecification{};
        l_LightingShaderSpecification.Modules.push_back({ ShaderStage::Vertex, "lighting_pass.vert.spv" });
        l_LightingShaderSpecification.Modules.push_back({ ShaderStage::Fragment, "lighting_pass.frag.spv" });
        l_LightingShaderSpecification.DebugName = "LightingShader";
        auto a_LightingShader = Renderer::GetAPI().CreateShader(l_LightingShaderSpecification);

        PipelineSpecification l_LightingPipelineSpecification{};
        l_LightingPipelineSpecification.PipelineShader = a_LightingShader;
        l_LightingPipelineSpecification.ColorAttachmentFormats = { TextureFormat::RGBA8 };
        l_LightingPipelineSpecification.DepthTest = false;
        l_LightingPipelineSpecification.DepthWrite = false;
        l_LightingPipelineSpecification.CullingMode = CullMode::None;
        l_LightingPipelineSpecification.DescriptorBindings = {
            { 0, DescriptorType::CombinedImageSampler, ShaderStage::Fragment, 1 },
            { 1, DescriptorType::CombinedImageSampler, ShaderStage::Fragment, 1 },
            { 2, DescriptorType::CombinedImageSampler, ShaderStage::Fragment, 1 },
            { 3, DescriptorType::CombinedImageSampler, ShaderStage::Fragment, 1 },
            { 4, DescriptorType::CombinedImageSampler, ShaderStage::Fragment, 1 },
            { 5, DescriptorType::UniformBuffer, ShaderStage::Fragment, 1 },
            { 6, DescriptorType::StorageBuffer, ShaderStage::Fragment, 1 }
        };
        l_LightingPipelineSpecification.DebugName = "LightingPipeline";
        m_Implementation->LightingPipeline = Renderer::GetAPI().CreatePipeline(l_LightingPipelineSpecification);

        m_Implementation->Device = a_VulkanAPI->GetDevice().GetDevice();

        auto* a_VkGeomPipeline = dynamic_cast<VulkanPipeline*>(m_Implementation->GeometryPipeline.get());
        m_Implementation->GeometryDescriptorLayout = a_VkGeomPipeline->GetDescriptorSetLayout();

        auto* a_VkLightingPipeline = dynamic_cast<VulkanPipeline*>(m_Implementation->LightingPipeline.get());
        m_Implementation->LightingDescriptorLayout = a_VkLightingPipeline->GetDescriptorSetLayout();

        std::array<VkDescriptorPoolSize, 3> l_PoolSizes{};
        l_PoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_PoolSizes[0].descriptorCount = 256;
        l_PoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_PoolSizes[1].descriptorCount = 8;
        l_PoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        l_PoolSizes[2].descriptorCount = 8;

        VkDescriptorPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_PoolInfo.maxSets = 272;
        l_PoolInfo.poolSizeCount = static_cast<uint32_t>(l_PoolSizes.size());
        l_PoolInfo.pPoolSizes = l_PoolSizes.data();
        VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Implementation->Device, &l_PoolInfo, nullptr, &m_Implementation->DescriptorPool), "Failed vkCreateDescriptorPool");

        SamplerSpecification l_SamplerSpecification{};
        l_SamplerSpecification.MinFilter = SamplerFilter::Linear;
        l_SamplerSpecification.MagFilter = SamplerFilter::Linear;
        l_SamplerSpecification.MipmapMode = SamplerMipmapMode::Linear;
        l_SamplerSpecification.AddressModeU = SamplerAddressMode::Repeat;
        l_SamplerSpecification.AddressModeV = SamplerAddressMode::Repeat;
        l_SamplerSpecification.AddressModeW = SamplerAddressMode::Repeat;
        l_SamplerSpecification.DebugName = "AlbedoSampler";
        m_Implementation->AlbedoSampler = Renderer::GetAPI().CreateSampler(l_SamplerSpecification);

        SamplerSpecification l_GeometryBufferSamplerSpecification{};
        l_GeometryBufferSamplerSpecification.MinFilter = SamplerFilter::Linear;
        l_GeometryBufferSamplerSpecification.MagFilter = SamplerFilter::Linear;
        l_GeometryBufferSamplerSpecification.MipmapMode = SamplerMipmapMode::Linear;
        l_GeometryBufferSamplerSpecification.AddressModeU = SamplerAddressMode::ClampToEdge;
        l_GeometryBufferSamplerSpecification.AddressModeV = SamplerAddressMode::ClampToEdge;
        l_GeometryBufferSamplerSpecification.AddressModeW = SamplerAddressMode::ClampToEdge;
        l_GeometryBufferSamplerSpecification.DebugName = "GeometryBufferSampler";
        m_Implementation->GBufferSampler = Renderer::GetAPI().CreateSampler(l_GeometryBufferSamplerSpecification);

        const uint32_t l_MaxFrames = Renderer::GetAPI().GetMaxFramesInFlight();
        m_Implementation->LightingUniformBuffers.resize(l_MaxFrames);
        m_Implementation->LightingDescriptorSets.resize(l_MaxFrames, VK_NULL_HANDLE);

        BufferSpecification l_LightBufferSpecification{};
        l_LightBufferSpecification.Size = static_cast<uint64_t>(s_MaxSupportedSceneLights) * sizeof(GpuLight);
        l_LightBufferSpecification.Usage = BufferUsage::StorageBuffer;
        l_LightBufferSpecification.MemoryType = BufferMemoryType::CPUToGPU;
        l_LightBufferSpecification.DebugName = "SceneLightsStorage";
        m_Implementation->LightStorageBuffer = Renderer::GetAPI().CreateBuffer(l_LightBufferSpecification);

        for (uint32_t i = 0; i < l_MaxFrames; i++)
        {
            BufferSpecification l_UBOSpec{};
            l_UBOSpec.Size = sizeof(LightingUniformData);
            l_UBOSpec.Usage = BufferUsage::UniformBuffer;
            l_UBOSpec.MemoryType = BufferMemoryType::CPUToGPU;
            l_UBOSpec.DebugName = "LightingUniforms";
            m_Implementation->LightingUniformBuffers[i] = Renderer::GetAPI().CreateBuffer(l_UBOSpec);

            VkDescriptorSetAllocateInfo l_LightingAllocInfo{};
            l_LightingAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            l_LightingAllocInfo.descriptorPool = m_Implementation->DescriptorPool;
            l_LightingAllocInfo.descriptorSetCount = 1;
            l_LightingAllocInfo.pSetLayouts = &m_Implementation->LightingDescriptorLayout;
            VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Implementation->Device, &l_LightingAllocInfo, &m_Implementation->LightingDescriptorSets[i]), "Failed vkAllocateDescriptorSets for lighting");
        }

        SetupFallbackTextureAndSampler();
    }

    void VulkanSceneRenderer::Shutdown()
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

    void VulkanSceneRenderer::BeginScene(const Camera& camera, const SceneRenderData& sceneData)
    {
        m_Camera = camera;
        m_SceneData = sceneData;
        m_DrawList.clear();
        m_Implementation->Stats = {};
        m_Implementation->SceneActive = true;
    }

    void VulkanSceneRenderer::SubmitMesh(const MeshDrawCommand& command)
    {
        m_DrawList.push_back(command);
    }

    void VulkanSceneRenderer::EndScene()
    {
        m_Implementation->SceneActive = false;
    }

    void VulkanSceneRenderer::Render()
    {
        if (!m_Implementation || !m_Implementation->RenderGraph || !m_Implementation->GeometryPipeline || !m_Implementation->ShadowPipeline)
        {
            return;
        }

        m_Implementation->RenderGraph->Execute();
    }

    void VulkanSceneRenderer::OnResize(uint32_t width, uint32_t height)
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

    std::shared_ptr<Texture> VulkanSceneRenderer::GetFinalOutput() const
    {
        if (!m_Implementation || !m_Implementation->RenderGraph)
        {
            return nullptr;
        }

        return m_Implementation->RenderGraph->GetTexture(m_Implementation->LitOutputHandle);
    }

    const SceneRendererStats& VulkanSceneRenderer::GetStats() const
    {
        return m_Implementation->Stats;
    }


    std::unique_ptr<ISceneRenderer> CreateVulkanSceneRenderer()
    {
        return std::make_unique<VulkanSceneRenderer>();
    }
}
