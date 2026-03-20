#pragma once

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/ShaderLibrary.h"
#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanAllocator.h"
#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"
#include "Trinity/Renderer/Vulkan/VulkanSync.h"
#include "Trinity/Renderer/Vulkan/VulkanCommand.h"
#include "Trinity/Renderer/Vulkan/VulkanUploadContext.h"
#include "Trinity/Renderer/Vulkan/VulkanResourceStateTracker.h"
#include "Trinity/Renderer/Vulkan/VulkanRenderGraph.h"
#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/VulkanPipeline.h"
#include "Trinity/Geometry/Geometry.h"

#include <array>
#include <cstdint>
#include <memory>

namespace Trinity
{
    class Window;
    class ImGuiLayer;

    class VulkanRenderer : public Renderer
    {
    public:
        struct Configuration
        {
            VulkanSwapchain::ColorOutputPolicy m_ColorOutputPolicy = VulkanSwapchain::ColorOutputPolicy::SDRsRGB;
        };

        VulkanRenderer();
        explicit VulkanRenderer(const Configuration& configuration);

        void SetWindow(Window& window) override;
        void SetConfiguration(const Configuration& configuration);

        void Initialize() override;
        void Shutdown() override;

        void Resize(uint32_t width, uint32_t height) override;

        void BeginFrame() override;
        void EndFrame() override;

        void RenderImGui(ImGuiLayer& imGuiLayer) override;
        void SetSceneViewportSize(uint32_t width, uint32_t height) override;
        void* GetSceneViewportHandle() const override;

        // Forward mesh draws for the simple (non-deferred) forward pipeline
        void DrawMesh(Geometry::PrimitiveType primitive, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection);
        void DrawMesh(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection);

        VulkanRenderGraph& GetRenderGraph() { return m_RenderGraph; }
        const VulkanRenderGraph& GetRenderGraph() const { return m_RenderGraph; }

        // Internal accessors used by ImGuiLayer
        VkInstance GetVulkanInstance() const { return m_Context.GetInstance(); }
        VkAllocationCallbacks* GetVulkanAllocator() const { return m_Context.GetAllocator(); }
        VkPhysicalDevice GetVulkanPhysicalDevice() const { return m_Device.GetPhysicalDevice(); }
        VkDevice GetVulkanDevice() const { return m_Device.GetDevice(); }
        VkQueue GetVulkanGraphicsQueue() const { return m_Device.GetGraphicsQueue(); }
        uint32_t GetVulkanGraphicsQueueFamilyIndex() const { return m_Device.GetGraphicsQueueFamilyIndex(); }
        uint32_t GetVulkanSwapchainImageCount() const { return m_Swapchain.GetImageCount(); }
        VkFormat GetVulkanSwapchainImageFormat() const { return m_Swapchain.GetImageFormat(); }
        VkCommandPool GetVulkanCommandPool(uint32_t frameIndex) const { return m_Command.GetCommandPool(frameIndex); }

        VulkanAllocator& GetVMAAllocator() { return m_Allocator; }
        VulkanUploadContext& GetUploadContext() { return m_UploadContext; }

        bool IsFrameBegun() const { return m_FrameBegun; }

    private:
        void RecreateSwapchain(uint32_t width, uint32_t height);
        void RecreateViewport();
        void BuildPassContext(VulkanPassContext& outContext) const;
        void WirePassResources();
        void InitializePasses();
        void EnsurePrimitiveUploaded(Geometry::PrimitiveType primitive);

        static VkImageSubresourceRange BuildColorSubresourceRange();

    private:
        struct PrimitiveGpu
        {
            std::unique_ptr<VulkanVertexBuffer> VulkanVB;
            std::unique_ptr<VulkanIndexBuffer>  VulkanIB;
        };

        Window* m_Window = nullptr;

        ShaderLibrary m_ShaderLibrary{};

        VulkanContext m_Context{};
        VulkanDevice m_Device{};
        VulkanAllocator m_Allocator{};
        VulkanSwapchain m_Swapchain{};
        VulkanSync m_Sync{};
        VulkanCommand m_Command{};
        VulkanUploadContext m_UploadContext{};
        VulkanResourceStateTracker m_ResourceStateTracker{};

        VulkanRenderGraph m_RenderGraph{};

        uint32_t m_FramesInFlight = 2;
        uint32_t m_CurrentFrameIndex = 0;
        uint32_t m_CurrentImageIndex = 0;
        bool m_FrameBegun = false;

        uint32_t m_SceneViewportWidth = 0;
        uint32_t m_SceneViewportHeight = 0;
        bool m_PendingViewportRecreate = false;
        uint32_t m_PendingWidth = 0;
        uint32_t m_PendingHeight = 0;

        // Simple forward pipeline (non-deferred draw calls)
        VulkanPipeline m_Pipeline{};
        bool m_ScenePassRecording = false;
        bool m_PresentPassRecording = false;

        ImGuiLayer* m_ImGuiLayer = nullptr;

        std::array<PrimitiveGpu, static_cast<size_t>(Geometry::PrimitiveType::Count)> m_Primitives{};

        Configuration m_Configuration{};
    };
}