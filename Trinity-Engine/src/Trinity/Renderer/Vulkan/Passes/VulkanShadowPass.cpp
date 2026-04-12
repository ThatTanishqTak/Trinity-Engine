#include "Trinity/Renderer/Vulkan/Passes/VulkanShadowPass.h"

#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

namespace Trinity
{
    VulkanShadowPass::VulkanShadowPass(VulkanRendererAPI& renderer, uint32_t shadowMapSize) : m_Renderer(&renderer), m_ShadowMapSize(shadowMapSize)
    {
        m_Name = "ShadowPass";

        FramebufferSpecification l_FramebufferSpecification{};
        l_FramebufferSpecification.Width = shadowMapSize;
        l_FramebufferSpecification.Height = shadowMapSize;
        l_FramebufferSpecification.HasDepthAttachment = true;
        l_FramebufferSpecification.DepthAttachment.Format = TextureFormat::Depth32F;
        l_FramebufferSpecification.DepthAttachment.DebugName = "ShadowMap-Depth";
        l_FramebufferSpecification.DebugName = "ShadowPass-Framebuffer";

        m_Framebuffer = std::make_shared<VulkanFramebuffer>(renderer.GetDevice().GetDevice(), renderer.GetAllocator().GetAllocator(), l_FramebufferSpecification);
    }

    void VulkanShadowPass::Begin(uint32_t width, uint32_t height)
    {
        VkCommandBuffer l_Cmd = m_Renderer->GetCurrentCommandBuffer();

        auto a_DepthTexture = std::dynamic_pointer_cast<VulkanTexture>(m_Framebuffer->GetDepthAttachment());

        VkRenderingAttachmentInfo l_DepthAttachment{};
        l_DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        l_DepthAttachment.imageView = a_DepthTexture->GetImageView();
        l_DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        l_DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        l_DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        l_DepthAttachment.clearValue.depthStencil = { 1.0f, 0 };

        VkRenderingInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        l_RenderingInfo.renderArea = { { 0, 0 }, { m_ShadowMapSize, m_ShadowMapSize } };
        l_RenderingInfo.layerCount = 1;
        l_RenderingInfo.colorAttachmentCount = 0;
        l_RenderingInfo.pDepthAttachment = &l_DepthAttachment;

        vkCmdBeginRendering(l_Cmd, &l_RenderingInfo);

        VkViewport l_Viewport{};
        l_Viewport.x = 0.0f;
        l_Viewport.y = 0.0f;
        l_Viewport.width = static_cast<float>(m_ShadowMapSize);
        l_Viewport.height = static_cast<float>(m_ShadowMapSize);
        l_Viewport.minDepth = 0.0f;
        l_Viewport.maxDepth = 1.0f;
        vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

        VkRect2D l_Scissor{};
        l_Scissor.offset = { 0, 0 };
        l_Scissor.extent = { m_ShadowMapSize, m_ShadowMapSize };
        vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);
    }

    void VulkanShadowPass::End()
    {
        VkCommandBuffer l_Cmd = m_Renderer->GetCurrentCommandBuffer();
        vkCmdEndRendering(l_Cmd);
    }

    void VulkanShadowPass::Resize(uint32_t width, uint32_t height)
    {
        m_ShadowMapSize = width;
        m_Framebuffer->Resize(width, height);
    }

    std::shared_ptr<Texture> VulkanShadowPass::GetShadowMap() const
    {
        return m_Framebuffer->GetDepthAttachment();
    }
}