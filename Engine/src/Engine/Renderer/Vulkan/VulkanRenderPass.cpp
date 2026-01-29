#include "Engine/Renderer/Vulkan/VulkanRenderPass.h"

#include "Engine/Utilities/Utilities.h"

namespace Engine
{
    void VulkanRenderPass::Initialize(VkDevice device)
    {
        m_Device = device;
    }

    void VulkanRenderPass::Shutdown()
    {
        CleanupSwapchainPass();

        m_Device = VK_NULL_HANDLE;
        m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
        m_SwapchainRenderPass = VK_NULL_HANDLE;
    }

    void VulkanRenderPass::CreateSwapchainPass(VkFormat swapchainImageFormat)
    {
        m_SwapchainImageFormat = swapchainImageFormat;

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

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateRenderPass(m_Device, &l_RPInfo, nullptr, &m_SwapchainRenderPass), "vkCreateRenderPass");
    }

    void VulkanRenderPass::CleanupSwapchainPass()
    {
        if (m_SwapchainRenderPass)
        {
            vkDestroyRenderPass(m_Device, m_SwapchainRenderPass, nullptr);
            m_SwapchainRenderPass = VK_NULL_HANDLE;
        }
    }

    VkRenderPass VulkanRenderPass::GetSwapchainRenderPass() const
    {
        return m_SwapchainRenderPass;
    }
}