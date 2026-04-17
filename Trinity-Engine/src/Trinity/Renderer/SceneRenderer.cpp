#include "Trinity/Renderer/SceneRenderer.h"

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Mesh.h"
#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#include "Trinity/Renderer/Vulkan/Passes/VulkanGeometryPass.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Utilities/Log.h"

#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.h>

#include <cstring>
#include <memory>

namespace Trinity
{
    namespace
    {
        static void TransitionImage(
            VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect,
            VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
            VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess)
        {
            VkImageMemoryBarrier2 l_Barrier{};
            l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            l_Barrier.srcStageMask  = srcStage;
            l_Barrier.srcAccessMask = srcAccess;
            l_Barrier.dstStageMask  = dstStage;
            l_Barrier.dstAccessMask = dstAccess;
            l_Barrier.oldLayout = oldLayout;
            l_Barrier.newLayout = newLayout;
            l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            l_Barrier.image = image;
            l_Barrier.subresourceRange = { aspect, 0, 1, 0, 1 };

            VkDependencyInfo l_Dep{};
            l_Dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            l_Dep.imageMemoryBarrierCount = 1;
            l_Dep.pImageMemoryBarriers = &l_Barrier;

            vkCmdPipelineBarrier2(cmd, &l_Dep);
        }
    }

    struct SceneRenderer::Implementation
    {
        SceneRendererStats Stats;
        bool SceneActive = false;
        std::unique_ptr<VulkanGeometryPass> GeometryPass;
        std::shared_ptr<Pipeline> MeshPipeline;
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

        m_Implementation->GeometryPass = std::make_unique<VulkanGeometryPass>(*a_VkAPI, width, height);

        ShaderSpecification l_ShaderSpec{};
        l_ShaderSpec.Modules.push_back({ ShaderStage::Vertex,   "geometry_pass.vert.spv" });
        l_ShaderSpec.Modules.push_back({ ShaderStage::Fragment, "geometry_pass.frag.spv" });
        l_ShaderSpec.DebugName = "MeshShader";
        auto l_Shader = Renderer::GetAPI().CreateShader(l_ShaderSpec);

        PipelineSpecification l_PipelineSpec{};
        l_PipelineSpec.PipelineShader = l_Shader;
        l_PipelineSpec.VertexAttributes = {
            { 0, 0, VertexAttributeFormat::Float3,  0 },   // Position
            { 1, 0, VertexAttributeFormat::Float3, 12 },   // Normal
            { 2, 0, VertexAttributeFormat::Float2, 24 },   // TexCoord
        };
        l_PipelineSpec.VertexStride = sizeof(Vertex);                     // 32 bytes
        l_PipelineSpec.PushConstants = {{ ShaderStage::Vertex, 0, 128 }}; // model (64) + VP (64)
        l_PipelineSpec.ColorAttachmentFormats = { TextureFormat::RGBA16F, TextureFormat::RGBA16F };
        l_PipelineSpec.DepthAttachmentFormat  = TextureFormat::Depth32F;
        l_PipelineSpec.DepthTest   = true;
        l_PipelineSpec.DepthWrite  = true;
        l_PipelineSpec.DepthOp     = DepthCompareOp::Less;
        l_PipelineSpec.CullingMode = CullMode::Back;
        l_PipelineSpec.DebugName   = "MeshPipeline";
        m_Implementation->MeshPipeline = Renderer::GetAPI().CreatePipeline(l_PipelineSpec);
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
        m_Implementation->Stats      = {};
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
        if (!m_Implementation || !m_Implementation->GeometryPass || !m_Implementation->MeshPipeline)
        {
            return;
        }

        auto* a_VkAPI = dynamic_cast<VulkanRendererAPI*>(&Renderer::GetAPI());
        VkCommandBuffer l_Cmd = a_VkAPI->GetCurrentCommandBuffer();

        auto l_Fb = m_Implementation->GeometryPass->GetFramebuffer();

        // Transition all color attachments: UNDEFINED → COLOR_ATTACHMENT_OPTIMAL (contents discarded — fine, we clear)
        for (uint32_t i = 0; i < l_Fb->GetColorAttachmentCount(); i++)
        {
            auto l_Tex = std::dynamic_pointer_cast<VulkanTexture>(l_Fb->GetColorAttachment(i));
            TransitionImage(l_Cmd, l_Tex->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,        0,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        }

        // Transition depth: UNDEFINED → DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        {
            auto l_Depth = std::dynamic_pointer_cast<VulkanTexture>(l_Fb->GetDepthAttachment());
            TransitionImage(l_Cmd, l_Depth->GetImage(), VK_IMAGE_ASPECT_DEPTH_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,             0,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,   VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
        }

        m_Implementation->GeometryPass->Begin(m_Width, m_Height);

        if (!m_DrawList.empty())
        {
            auto* a_VkPipeline = dynamic_cast<VulkanPipeline*>(m_Implementation->MeshPipeline.get());
            vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, a_VkPipeline->GetPipeline());

            const glm::mat4 l_VP = m_Camera.GetViewProjectionMatrix();

            for (const auto& cmd : m_DrawList)
            {
                if (!cmd.MeshRef) continue;

                auto* a_VB = dynamic_cast<VulkanBuffer*>(cmd.MeshRef->GetVertexBuffer().get());
                auto* a_IB = dynamic_cast<VulkanBuffer*>(cmd.MeshRef->GetIndexBuffer().get());
                if (!a_VB || !a_IB) continue;

                VkBuffer     l_Buffers[] = { a_VB->GetBuffer() };
                VkDeviceSize l_Offsets[] = { 0 };
                vkCmdBindVertexBuffers(l_Cmd, 0, 1, l_Buffers, l_Offsets);
                vkCmdBindIndexBuffer(l_Cmd, a_IB->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

                struct { float Model[16]; float VP[16]; } l_Push;
                std::memcpy(l_Push.Model, cmd.Transform,            64);
                std::memcpy(l_Push.VP,    glm::value_ptr(l_VP),     64);
                vkCmdPushConstants(l_Cmd, a_VkPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, 128, &l_Push);

                vkCmdDrawIndexed(l_Cmd, cmd.MeshRef->GetIndexCount(), 1, 0, 0, 0);

                m_Implementation->Stats.DrawCalls++;
                m_Implementation->Stats.IndexCount  += cmd.MeshRef->GetIndexCount();
                m_Implementation->Stats.VertexCount += cmd.MeshRef->GetVertexCount();
            }
        }

        m_Implementation->GeometryPass->End();

        // Transition color[0]: COLOR_ATTACHMENT_OPTIMAL → SHADER_READ_ONLY_OPTIMAL so ImGui can sample it
        {
            auto l_Color = std::dynamic_pointer_cast<VulkanTexture>(l_Fb->GetColorAttachment(0));
            TransitionImage(l_Cmd, l_Color->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,         VK_ACCESS_2_SHADER_READ_BIT);
        }
    }

    void SceneRenderer::OnResize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0) return;
        if (m_Width == width && m_Height == height) return;

        // Wait for the GPU to finish before destroying framebuffer images
        Renderer::WaitIdle();

        m_Width  = width;
        m_Height = height;

        if (m_Implementation && m_Implementation->GeometryPass)
        {
            m_Implementation->GeometryPass->Resize(width, height);
        }
    }

    std::shared_ptr<Texture> SceneRenderer::GetFinalOutput() const
    {
        if (!m_Implementation || !m_Implementation->GeometryPass)
        {
            return nullptr;
        }

        return m_Implementation->GeometryPass->GetFramebuffer()->GetColorAttachment(0);
    }

    const SceneRendererStats& SceneRenderer::GetStats() const
    {
        return m_Implementation->Stats;
    }
}
