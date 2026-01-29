#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace Engine
{
    class VulkanFramebuffers
    {
    public:
        void Initialize(VkDevice device, VkFormat swapchainImageFormat, const std::vector<VkImageView>& swapchainImageViews);
        void Shutdown();

        void Create(VkExtent2D swapchainExtent);
        void Cleanup();

        VkRenderPass GetRenderPass() const { return m_RenderPass; }
        const std::vector<VkFramebuffer>& GetFramebuffers() const { return m_Framebuffers; }

    private:
        void CreateRenderPass();
        void CreateFramebuffers(VkExtent2D swapchainExtent);

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
        std::vector<VkImageView> m_SwapchainImageViews;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_Framebuffers;
    };
}