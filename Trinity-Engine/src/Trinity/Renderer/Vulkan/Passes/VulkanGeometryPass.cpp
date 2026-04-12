#include "Trinity/Renderer/Vulkan/Passes/VulkanGeometryPass.h"

#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

namespace Trinity
{
    VulkanGeometryPass::VulkanGeometryPass(VulkanRendererAPI& renderer) : m_Renderer(&renderer)
    {
        m_Name = "GeometryPass";

        FramebufferSpecification l_FramebufferSpecification{};
        l_FramebufferSpecification.Width = renderer.GetSwapchainWidth();
        l_FramebufferSpecification.Height = renderer.GetSwapchainHeight();

        FramebufferAttachmentSpecification l_ColorAttach{};
        l_ColorAttach.Format = TextureFormat::RGBA16F;
        l_ColorAttach.DebugName = "GBuffer-Color";
        l_FramebufferSpecification.ColorAttachments.push_back(l_ColorAttach);

        FramebufferAttachmentSpecification l_NormalAttach{};
        l_NormalAttach.Format = TextureFormat::RGBA16F;
        l_NormalAttach.DebugName = "GBuffer-Normal";
        l_FramebufferSpecification.ColorAttachments.push_back(l_NormalAttach);

        l_FramebufferSpecification.HasDepthAttachment = true;
        l_FramebufferSpecification.DepthAttachment.Format = TextureFormat::Depth32F;
        l_FramebufferSpecification.DepthAttachment.DebugName = "GBuffer-Depth";
        l_FramebufferSpecification.DebugName = "GeometryPass-Framebuffer";

        m_Framebuffer = std::make_shared<VulkanFramebuffer>(renderer.GetDevice().GetDevice(), renderer.GetAllocator().GetAllocator(), l_FramebufferSpecification);
    }

    void VulkanGeometryPass::Begin(uint32_t width, uint32_t height)
    {
        if (m_Framebuffer->GetWidth() != width || m_Framebuffer->GetHeight() != height)
        {
            m_Framebuffer->Resize(width, height);
        }

        VkCommandBuffer l_Cmd = m_Renderer->GetCurrentCommandBuffer();

        std::vector<VkRenderingAttachmentInfo> l_ColorAttachments;
        for (uint32_t i = 0; i < m_Framebuffer->GetColorAttachmentCount(); i++)
        {
            auto a_Texture = std::dynamic_pointer_cast<VulkanTexture>(m_Framebuffer->GetColorAttachment(i));

            VkRenderingAttachmentInfo l_AttachInfo{};
            l_AttachInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            l_AttachInfo.imageView = a_Texture->GetImageView();
            l_AttachInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            l_AttachInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            l_AttachInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            l_AttachInfo.clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
            l_ColorAttachments.push_back(l_AttachInfo);
        }

        VkRenderingAttachmentInfo l_DepthAttachment{};
        l_DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

        auto a_DepthTexture = std::dynamic_pointer_cast<VulkanTexture>(m_Framebuffer->GetDepthAttachment());
        if (a_DepthTexture)
        {
            l_DepthAttachment.imageView = a_DepthTexture->GetImageView();
            l_DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            l_DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            l_DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            l_DepthAttachment.clearValue.depthStencil = { 1.0f, 0 };
        }

        VkRenderingInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        l_RenderingInfo.renderArea = { { 0, 0 }, { width, height } };
        l_RenderingInfo.layerCount = 1;
        l_RenderingInfo.colorAttachmentCount = static_cast<uint32_t>(l_ColorAttachments.size());
        l_RenderingInfo.pColorAttachments = l_ColorAttachments.data();
        l_RenderingInfo.pDepthAttachment = a_DepthTexture ? &l_DepthAttachment : nullptr;

        vkCmdBeginRendering(l_Cmd, &l_RenderingInfo);

        VkViewport l_Viewport{};
        l_Viewport.x = 0.0f;
        l_Viewport.y = 0.0f;
        l_Viewport.width = static_cast<float>(width);
        l_Viewport.height = static_cast<float>(height);
        l_Viewport.minDepth = 0.0f;
        l_Viewport.maxDepth = 1.0f;
        vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

        VkRect2D l_Scissor{};
        l_Scissor.offset = { 0, 0 };
        l_Scissor.extent = { width, height };
        vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);
    }

    void VulkanGeometryPass::End()
    {
        VkCommandBuffer l_Cmd = m_Renderer->GetCurrentCommandBuffer();
        vkCmdEndRendering(l_Cmd);
    }

    void VulkanGeometryPass::Resize(uint32_t width, uint32_t height)
    {
        m_Framebuffer->Resize(width, height);
    }
}