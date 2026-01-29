#pragma once

#include "Engine/Renderer/RendererAPI.h"

#include "Engine/Renderer/Vulkan/VulkanSwapchain.h"
#include "Engine/Renderer/Vulkan/VulkanDevice.h"
#include "Engine/Renderer/Vulkan/VulkanCommand.h"
#include "Engine/Renderer/Vulkan/VulkanSync.h"
#include "Engine/Renderer/Vulkan/VulkanResources.h"
#include "Engine/Renderer/Vulkan/VulkanPipeline.h"
#include "Engine/Renderer/Vulkan/VulkanDescriptors.h"

#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace Engine
{
    class Window;

    class VulkanRenderer final : public RendererAPI
    {
    public:
        API GetAPI() const override { return API::Vulkan; }

        void Initialize(Window* window) override;
        void Shutdown() override;

        void BeginFrame() override;
        void Execute(const std::vector<Command>& commandList) override;
        void EndFrame() override;

        void OnResize(uint32_t width, uint32_t height) override;
        void WaitIdle() override;

    private:
        struct UniformBufferObject
        {
            glm::mat4 MVP;
        };

    private:
        void CreateRenderPass();
        void CreatePipeline();
        void CreateFramebuffers();

        void CreateTriangleResources();
        void DestroyTriangleResources();

        void CreateUniformBuffers();
        void DestroyUniformBuffers();
        void UpdateUniformBuffer(uint32_t frameIndex);

        void CreateDescriptors();
        void DestroyDescriptors();

        void CleanupSwapchain();
        void RecreateSwapchain();

        void RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, const std::vector<Command>& commandList);

    private:
        Window* m_Window = nullptr;
        GLFWwindow* m_NativeWindow = nullptr;

        VulkanDevice m_VulkanDevice;

        VulkanSwapchain m_Swapchain;
        VulkanCommand m_Command;
        VulkanSync m_Sync;
        VulkanResources m_Resources;
        VulkanDescriptors m_Descriptors;
        VulkanPipeline m_Pipeline;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_Framebuffers;

        static constexpr uint32_t s_MaxFramesInFlight = 2;

        uint32_t m_CurrentFrame = 0;
        uint32_t m_CurrentImageIndex = 0;
        bool m_FramebufferResized = false;

        VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;

        std::vector<VkBuffer> m_UniformBuffers;
        std::vector<VkDeviceMemory> m_UniformBuffersMemory;
        std::vector<void*> m_UniformBuffersMapped;
        std::vector<VkDescriptorSet> m_DescriptorSets;

        glm::vec4 m_LastClearColor = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
    };
}