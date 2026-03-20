#pragma once

#include "Trinity/Renderer/RenderPass.h"
#include "Trinity/Renderer/Vulkan/VulkanPassContext.h"
#include "Trinity/Renderer/Vulkan/VulkanFrameContext.h"

#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanRenderPass : public RenderPass
    {
    public:
        virtual ~VulkanRenderPass() = default;

        virtual void Initialize(const VulkanPassContext& context) = 0;
        virtual void Execute(const VulkanFrameContext& frameContext) = 0;

    protected:
        void CopyContext(const VulkanPassContext& context)
        {
            m_Device = context.Device;
            m_PhysicalDevice = context.PhysicalDevice;
            m_HostAllocator = context.HostAllocator;
            m_Allocator = context.Allocator;
            m_UploadContext = context.UploadContext;
            m_ResourceStateTracker = context.ResourceStateTracker;
            m_Shaders = context.Shaders;
            m_GraphicsQueue = context.GraphicsQueue;
            m_GraphicsQueueFamilyIndex = context.GraphicsQueueFamilyIndex;
            m_SwapchainFormat = context.SwapchainFormat;
            m_DepthFormat = context.DepthFormat;
            m_FramesInFlight = context.FramesInFlight;
            m_ColorPolicy = context.ColorPolicy;
        }

    protected:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkAllocationCallbacks* m_HostAllocator = nullptr;
        VulkanAllocator* m_Allocator = nullptr;
        VulkanUploadContext* m_UploadContext = nullptr;
        VulkanResourceStateTracker* m_ResourceStateTracker = nullptr;
        ShaderLibrary* m_Shaders = nullptr;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        uint32_t m_GraphicsQueueFamilyIndex = 0;
        VkFormat m_SwapchainFormat = VK_FORMAT_UNDEFINED;
        VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
        uint32_t m_FramesInFlight = 0;
        VulkanSwapchain::SceneColorPolicy m_ColorPolicy{};
    };
}