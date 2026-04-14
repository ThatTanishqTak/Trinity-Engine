#pragma once

#include "Trinity/Renderer/RendererAPI.h"

#include "Trinity/Renderer/Vulkan/VulkanAllocator.h"
#include "Trinity/Renderer/Vulkan/VulkanCommandPool.h"
#include "Trinity/Renderer/Vulkan/VulkanDebug.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanInstance.h"
#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"
#include "Trinity/Renderer/Vulkan/VulkanSyncObjects.h"

#include <cstdint>

namespace Trinity
{
	class VulkanRendererAPI final : public RendererAPI
	{
    public:
        VulkanRendererAPI();
        ~VulkanRendererAPI() override;

        void Initialize(Window& window, const RendererAPISpecification& specification) override;
        void Shutdown() override;

        bool BeginFrame() override;
        void EndFrame() override;
        void Present() override;
        void WaitIdle() override;

        std::shared_ptr<Buffer> CreateBuffer(const BufferSpecification& specification) override;
        std::shared_ptr<Texture> CreateTexture(const TextureSpecification& specification) override;
        std::shared_ptr<Framebuffer> CreateFramebuffer(const FramebufferSpecification& specification) override;
        std::shared_ptr<Shader> CreateShader(const ShaderSpecification& specification) override;
        std::shared_ptr<Pipeline> CreatePipeline(const PipelineSpecification& specification) override;
        std::shared_ptr<Sampler> CreateSampler(const SamplerSpecification& specification) override;

        void OnWindowResize(uint32_t width, uint32_t height) override;
        uint32_t GetSwapchainWidth() const override;
        uint32_t GetSwapchainHeight() const override;
        uint32_t GetCurrentFrameIndex() const override { return m_CurrentFrameIndex; }
        uint32_t GetMaxFramesInFlight() const override { return m_MaxFramesInFlight; }

        VulkanDevice& GetDevice() { return m_Device; }
        VulkanAllocator& GetAllocator() { return m_Allocator; }
        VulkanCommandPool& GetCommandPool() { return m_CommandPool; }
        VkCommandBuffer GetCurrentCommandBuffer() const;
        uint32_t GetCurrentImageIndex() const { return m_CurrentImageIndex; }

    private:
        void TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) const;

    private:
        Window* m_Window = nullptr;

        VulkanInstance m_Instance;
        VulkanDevice m_Device;
        VulkanDebug m_Debug;
        VulkanAllocator m_Allocator;
        VulkanSwapchain m_Swapchain;
        VulkanCommandPool m_CommandPool;
        VulkanSyncObjects m_SyncObjects;

        uint32_t m_MaxFramesInFlight = 2;
        uint32_t m_CurrentFrameIndex = 0;
        uint32_t m_CurrentImageIndex = 0;
        bool m_FramebufferResized = false;
        bool m_FrameStarted = false;
    };
}