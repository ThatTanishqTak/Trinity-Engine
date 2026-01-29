#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace Engine
{
    class VulkanFramebuffers
    {
    public:
        void Initialize(VkDevice device);
        void SetSwapchainViews(VkFormat format, const std::vector<VkImageView>& views);
        void Shutdown();

        void Create(VkRenderPass renderPass, VkExtent2D extent, VkImageView depthView);
        void Cleanup();

        const std::vector<VkFramebuffer>& GetFramebuffers() const { return m_Framebuffers; }

    private:
        void CreateFramebuffers(VkRenderPass renderPass, VkExtent2D extent, VkImageView depthView);

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
        std::vector<VkImageView> m_SwapchainImageViews;

        std::vector<VkFramebuffer> m_Framebuffers;
    };
}