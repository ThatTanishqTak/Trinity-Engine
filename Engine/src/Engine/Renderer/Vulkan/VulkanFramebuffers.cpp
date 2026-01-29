#include "Engine/Renderer/Vulkan/VulkanFramebuffers.h"

#include "Engine/Utilities/Utilities.h"

namespace Engine
{
    void VulkanFramebuffers::Initialize(VkDevice device)
    {
        m_Device = device;
    }

    void VulkanFramebuffers::SetSwapchainViews(VkFormat format, const std::vector<VkImageView>& views)
    {
        m_SwapchainImageFormat = format;
        m_SwapchainImageViews = views;
    }

    void VulkanFramebuffers::Shutdown()
    {
        Cleanup();

        m_Device = VK_NULL_HANDLE;
        m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
        m_SwapchainImageViews.clear();
    }

    void VulkanFramebuffers::Create(VkRenderPass renderPass, VkExtent2D extent)
    {
        CreateFramebuffers(renderPass, extent);
    }

    void VulkanFramebuffers::Cleanup()
    {
        for (VkFramebuffer it_Framebuffer : m_Framebuffers)
        {
            vkDestroyFramebuffer(m_Device, it_Framebuffer, nullptr);
        }
        m_Framebuffers.clear();
    }

    void VulkanFramebuffers::CreateFramebuffers(VkRenderPass renderPass, VkExtent2D extent)
    {
        m_Framebuffers.resize(m_SwapchainImageViews.size());

        for (size_t l_Index = 0; l_Index < m_SwapchainImageViews.size(); ++l_Index)
        {
            VkImageView l_Attachments[] = { m_SwapchainImageViews[l_Index] };

            VkFramebufferCreateInfo l_FBInfo{};
            l_FBInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            l_FBInfo.renderPass = renderPass;
            l_FBInfo.attachmentCount = 1;
            l_FBInfo.pAttachments = l_Attachments;
            l_FBInfo.width = extent.width;
            l_FBInfo.height = extent.height;
            l_FBInfo.layers = 1;

            Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateFramebuffer(m_Device, &l_FBInfo, nullptr, &m_Framebuffers[l_Index]), "vkCreateFramebuffer");
        }
    }
}