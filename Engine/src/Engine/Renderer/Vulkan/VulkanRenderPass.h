#pragma once

#include <vulkan/vulkan.h>

namespace Engine
{
    class VulkanRenderPass
    {
    public:
        void Initialize(VkDevice device);
        void Shutdown();

        void CreateSwapchainPass(VkFormat swapchainImageFormat);
        void CleanupSwapchainPass();

        VkRenderPass GetSwapchainRenderPass() const;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
        VkRenderPass m_SwapchainRenderPass = VK_NULL_HANDLE;
    };
}