#include "Engine/Renderer/Vulkan/VulkanFramebuffers.h"

#include "Engine/Utilities/Utilities.h"

namespace Engine
{
    void VulkanFramebuffers::Initialize(VkDevice device, VkFormat swapchainImageFormat, const std::vector<VkImageView>& swapchainImageViews)
    {
        m_Device = device;
        m_SwapchainImageFormat = swapchainImageFormat;
        m_SwapchainImageViews = swapchainImageViews;
    }

    void VulkanFramebuffers::Shutdown()
    {
        Cleanup();

        m_Device = VK_NULL_HANDLE;
        m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
        m_SwapchainImageViews.clear();
    }

    void VulkanFramebuffers::Create(VkExtent2D swapchainExtent)
    {
        CreateRenderPass();
        CreateFramebuffers(swapchainExtent);
    }

    void VulkanFramebuffers::Cleanup()
    {
        for (VkFramebuffer it_Framebuffer : m_Framebuffers)
        {
            vkDestroyFramebuffer(m_Device, it_Framebuffer, nullptr);
        }
        m_Framebuffers.clear();

        if (m_RenderPass)
        {
            vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }
    }

    void VulkanFramebuffers::CreateRenderPass()
    {
        VkAttachmentDescription l_ColorAttachment{};
        l_ColorAttachment.format = m_SwapchainImageFormat;
        l_ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        l_ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        l_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        l_ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        l_ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        l_ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        l_ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference l_ColorRef{};
        l_ColorRef.attachment = 0;
        l_ColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription l_Subpass{};
        l_Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        l_Subpass.colorAttachmentCount = 1;
        l_Subpass.pColorAttachments = &l_ColorRef;

        VkSubpassDependency l_Dep{};
        l_Dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        l_Dep.dstSubpass = 0;
        l_Dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        l_Dep.srcAccessMask = 0;
        l_Dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        l_Dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo l_RPInfo{};
        l_RPInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        l_RPInfo.attachmentCount = 1;
        l_RPInfo.pAttachments = &l_ColorAttachment;
        l_RPInfo.subpassCount = 1;
        l_RPInfo.pSubpasses = &l_Subpass;
        l_RPInfo.dependencyCount = 1;
        l_RPInfo.pDependencies = &l_Dep;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateRenderPass(m_Device, &l_RPInfo, nullptr, &m_RenderPass), "vkCreateRenderPass");
    }

    void VulkanFramebuffers::CreateFramebuffers(VkExtent2D swapchainExtent)
    {
        m_Framebuffers.resize(m_SwapchainImageViews.size());

        for (size_t l_Index = 0; l_Index < m_SwapchainImageViews.size(); ++l_Index)
        {
            VkImageView l_Attachments[] = { m_SwapchainImageViews[l_Index] };

            VkFramebufferCreateInfo l_FBInfo{};
            l_FBInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            l_FBInfo.renderPass = m_RenderPass;
            l_FBInfo.attachmentCount = 1;
            l_FBInfo.pAttachments = l_Attachments;
            l_FBInfo.width = swapchainExtent.width;
            l_FBInfo.height = swapchainExtent.height;
            l_FBInfo.layers = 1;

            Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateFramebuffer(m_Device, &l_FBInfo, nullptr, &m_Framebuffers[l_Index]), "vkCreateFramebuffer");
        }
    }
}