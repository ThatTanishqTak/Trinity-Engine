#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"

#include "Trinity/Renderer/Vulkan/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/VulkanShaderInterop.h"
#include "Trinity/Renderer/Vulkan/VulkanPassContext.h"
#include "Trinity/Renderer/Vulkan/VulkanFrameContext.h"
#include "Trinity/Renderer/Vulkan/Passes/VulkanShadowPass.h"
#include "Trinity/Renderer/Vulkan/Passes/VulkanGeometryPass.h"
#include "Trinity/Renderer/Vulkan/Passes/VulkanLightingPass.h"
#include "Trinity/Renderer/Vulkan/Passes/VulkanPostProcessPass.h"
#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Platform/Window.h"
#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>

namespace Trinity
{
    VkImageSubresourceRange VulkanRenderer::BuildColorSubresourceRange()
    {
        VkImageSubresourceRange l_Range{};
        l_Range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        l_Range.baseMipLevel = 0;
        l_Range.levelCount = 1;
        l_Range.baseArrayLayer = 0;
        l_Range.layerCount = 1;

        return l_Range;
    }

    VulkanRenderer::VulkanRenderer() : Renderer(RendererAPI::VULKAN)
    {

    }

    VulkanRenderer::VulkanRenderer(const Configuration& configuration) : Renderer(RendererAPI::VULKAN), m_Configuration(configuration)
    {

    }

    void VulkanRenderer::SetWindow(Window& window)
    {
        m_Window = &window;
    }

    void VulkanRenderer::SetConfiguration(const Configuration& configuration)
    {
        m_Configuration = configuration;
    }

    void VulkanRenderer::Initialize()
    {
        TR_CORE_TRACE("Initializing VulkanRenderer");

        if (!m_Window)
        {
            TR_CORE_CRITICAL("VulkanRenderer::Initialize called without a window");
            std::abort();
        }

        const NativeWindowHandle l_NativeHandle = m_Window->GetNativeHandle();

        m_Context.Initialize(l_NativeHandle);
        m_Device.Initialize(m_Context);
        m_Allocator.Initialize(m_Context, m_Device);

        const VkFormat l_DepthFormat = Utilities::VulkanUtilities::FindDepthFormat(m_Device.GetPhysicalDevice());
        m_Swapchain.Initialize(m_Context, m_Device, m_Window->GetWidth(), m_Window->GetHeight(), m_Configuration.m_ColorOutputPolicy);
        m_Sync.Initialize(m_Context, m_Device, m_FramesInFlight, m_Swapchain.GetImageCount());
        m_Command.Initialize(m_Context, m_Device, m_FramesInFlight);
        m_UploadContext.Initialize(m_Context, m_Device);

        m_ShaderLibrary.Load("Simple.vert", "Assets/Shaders/Simple.vert.spv", ShaderStage::Vertex);
        m_ShaderLibrary.Load("Simple.frag", "Assets/Shaders/Simple.frag.spv", ShaderStage::Fragment);

        const std::vector<uint32_t>& l_VertSpirV = *m_ShaderLibrary.GetSpirV("Simple.vert");
        const std::vector<uint32_t>& l_FragSpirV = *m_ShaderLibrary.GetSpirV("Simple.frag");

        m_Pipeline.Initialize(m_Context, m_Device, m_Swapchain.GetImageFormat(), l_DepthFormat, l_VertSpirV, l_FragSpirV);

        m_ResourceStateTracker.Reset();

        InitializePasses();

        TR_CORE_TRACE("VulkanRenderer Initialized");
    }

    void VulkanRenderer::Shutdown()
    {
        TR_CORE_TRACE("Shutting Down VulkanRenderer");

        if (m_Device.GetDevice() != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device.GetDevice());
        }

        m_RenderGraph.Shutdown();

        for (auto& it_Primitive : m_Primitives)
        {
            it_Primitive.VulkanVB.reset();
            it_Primitive.VulkanIB.reset();
        }

        m_UploadContext.Shutdown();
        m_Pipeline.Shutdown();
        m_ShaderLibrary.Clear();
        m_Command.Shutdown();
        m_Sync.Shutdown();
        m_ResourceStateTracker.Reset();
        m_Swapchain.Shutdown();
        m_Allocator.Shutdown();
        m_Device.Shutdown();
        m_Context.Shutdown();

        m_ImGuiLayer = nullptr;

        TR_CORE_TRACE("VulkanRenderer Shutdown Complete");
    }

    void VulkanRenderer::Resize(uint32_t width, uint32_t height)
    {
        RecreateSwapchain(width, height);
    }

    void VulkanRenderer::SetSceneViewportSize(uint32_t width, uint32_t height)
    {
        if (m_SceneViewportWidth == width && m_SceneViewportHeight == height)
        {
            return;
        }

        m_PendingWidth = width;
        m_PendingHeight = height;
        m_PendingViewportRecreate = true;
    }

    void* VulkanRenderer::GetSceneViewportHandle() const
    {
        const auto* l_PostProcess = m_RenderGraph.GetPass<VulkanPostProcessPass>();
        if (!l_PostProcess)
        {
            return nullptr;
        }

        return l_PostProcess->GetOutputHandle();
    }

    void VulkanRenderer::BeginFrame()
    {
        if (!m_Window || m_Window->IsMinimized())
        {
            m_FrameBegun = false;

            return;
        }

        if (m_PendingViewportRecreate && m_Device.GetDevice() != VK_NULL_HANDLE)
        {
            for (uint32_t i = 0; i < m_FramesInFlight; ++i)
            {
                m_Sync.WaitForFrameFence(i);
            }

            m_SceneViewportWidth = m_PendingWidth;
            m_SceneViewportHeight = m_PendingHeight;

            RecreateViewport();
            m_PendingViewportRecreate = false;
        }

        m_FrameBegun = false;
        m_Sync.WaitForFrameFence(m_CurrentFrameIndex);

        const VkSemaphore l_ImageAvailable = m_Sync.GetImageAvailableSemaphore(m_CurrentFrameIndex);
        VkResult l_Acquire = m_Swapchain.AcquireNextImageIndex(l_ImageAvailable, m_CurrentImageIndex);

        if (l_Acquire == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain(m_Window->GetWidth(), m_Window->GetHeight());
            return;
        }

        if (l_Acquire != VK_SUCCESS && l_Acquire != VK_SUBOPTIMAL_KHR)
        {
            TR_CORE_CRITICAL("vkAcquireNextImageKHR failed (VkResult = {})", static_cast<int>(l_Acquire));
            std::abort();
        }

        const VkFence l_ImageFence = m_Sync.GetImageInFlightFence(m_CurrentImageIndex);
        if (l_ImageFence != VK_NULL_HANDLE)
        {
            Utilities::VulkanUtilities::VKCheck(vkWaitForFences(m_Device.GetDevice(), 1, &l_ImageFence, VK_TRUE, UINT64_MAX), "Failed vkWaitForFences");
        }

        m_Sync.SetImageInFlightFence(m_CurrentImageIndex, m_Sync.GetInFlightFence(m_CurrentFrameIndex));
        m_Sync.ResetFrameFence(m_CurrentFrameIndex);

        m_Command.Reset(m_CurrentFrameIndex);
        m_Command.Begin(m_CurrentFrameIndex);

        m_FrameBegun = true;
    }

    void VulkanRenderer::EndFrame()
    {
        if (!m_FrameBegun)
        {
            return;
        }

        const VkCommandBuffer l_CommandBuffer = m_Command.GetCommandBuffer(m_CurrentFrameIndex);
        const VkImageSubresourceRange l_ColorRange = BuildColorSubresourceRange();

        // Execute all render graph passes
        {
            VulkanFrameContext l_FrameCtx{};
            l_FrameCtx.CommandBuffer = l_CommandBuffer;
            l_FrameCtx.FrameIndex = m_CurrentFrameIndex;
            l_FrameCtx.ImageIndex = m_CurrentImageIndex;
            l_FrameCtx.ViewportWidth = m_SceneViewportWidth;
            l_FrameCtx.ViewportHeight = m_SceneViewportHeight;

            m_RenderGraph.Execute(l_FrameCtx);
        }

        // Present pass: blit/copy to swapchain (ImGui rendered on top)
        {
            m_ResourceStateTracker.TransitionImageResource(l_CommandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], l_ColorRange, g_ColorAttachmentWriteImageState);

            VkClearValue l_Clear{};
            l_Clear.color.float32[0] = 0.01f;
            l_Clear.color.float32[1] = 0.01f;
            l_Clear.color.float32[2] = 0.01f;
            l_Clear.color.float32[3] = 1.0f;

            VkRenderingAttachmentInfo l_ColorAtt{};
            l_ColorAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            l_ColorAtt.imageView = m_Swapchain.GetImageViews()[m_CurrentImageIndex];
            l_ColorAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            l_ColorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            l_ColorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            l_ColorAtt.clearValue = l_Clear;

            VkRenderingInfo l_RenderingInfo{};
            l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            l_RenderingInfo.renderArea.offset = { 0, 0 };
            l_RenderingInfo.renderArea.extent = m_Swapchain.GetExtent();
            l_RenderingInfo.layerCount = 1;
            l_RenderingInfo.colorAttachmentCount = 1;
            l_RenderingInfo.pColorAttachments = &l_ColorAtt;

            vkCmdBeginRendering(l_CommandBuffer, &l_RenderingInfo);

            if (m_ImGuiLayer != nullptr)
            {
                ImDrawData* l_DrawData = ImGui::GetDrawData();
                if (l_DrawData != nullptr)
                {
                    ImGui_ImplVulkan_RenderDrawData(l_DrawData, l_CommandBuffer);
                }
            }

            vkCmdEndRendering(l_CommandBuffer);

            m_ResourceStateTracker.TransitionImageResource(l_CommandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], l_ColorRange, g_PresentImageState);
        }

        m_Command.End(m_CurrentFrameIndex);

        const VkSemaphore l_ImageAvailable = m_Sync.GetImageAvailableSemaphore(m_CurrentFrameIndex);
        const VkSemaphore l_RenderFinished = m_Sync.GetRenderFinishedSemaphore(m_CurrentImageIndex);
        const VkFence l_InFlightFence = m_Sync.GetInFlightFence(m_CurrentFrameIndex);

        VkPipelineStageFlags l_WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        const VkCommandBuffer l_CB = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

        VkSubmitInfo l_Submit{};
        l_Submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_Submit.waitSemaphoreCount = 1;
        l_Submit.pWaitSemaphores = &l_ImageAvailable;
        l_Submit.pWaitDstStageMask = &l_WaitStage;
        l_Submit.commandBufferCount = 1;
        l_Submit.pCommandBuffers = &l_CB;
        l_Submit.signalSemaphoreCount = 1;
        l_Submit.pSignalSemaphores = &l_RenderFinished;

        Utilities::VulkanUtilities::VKCheck(vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &l_Submit, l_InFlightFence), "Failed vkQueueSubmit");

        VkResult l_Present = m_Swapchain.Present(m_Device.GetPresentQueue(), l_RenderFinished, m_CurrentImageIndex);
        if (l_Present == VK_ERROR_OUT_OF_DATE_KHR || l_Present == VK_SUBOPTIMAL_KHR)
        {
            RecreateSwapchain(m_Window->GetWidth(), m_Window->GetHeight());
        }
        else if (l_Present != VK_SUCCESS)
        {
            TR_CORE_CRITICAL("vkQueuePresentKHR failed (VkResult = {})", static_cast<int>(l_Present));
            std::abort();
        }

        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_FramesInFlight;
        m_FrameBegun = false;
    }

    void VulkanRenderer::RenderImGui(ImGuiLayer& imGuiLayer)
    {
        // Store ImGuiLayer reference so EndFrame can draw into the present pass.
        m_ImGuiLayer = &imGuiLayer;
    }

    // -----------------------------------------------------------------

    void VulkanRenderer::RecreateSwapchain(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0 || m_Device.GetDevice() == VK_NULL_HANDLE)
        {
            return;
        }

        vkDeviceWaitIdle(m_Device.GetDevice());

        const std::vector<VkImage> l_OldImages = m_Swapchain.GetImages();

        m_Swapchain.Recreate(width, height);
        m_Sync.OnSwapchainRecreated(m_Swapchain.GetImageCount());
        m_Pipeline.Recreate(m_Swapchain.GetImageFormat());

        if (m_ImGuiLayer != nullptr)
        {
            m_ImGuiLayer->OnSwapchainRecreated(m_Swapchain.GetImageCount(), m_Swapchain.GetImageFormat());
        }

        for (VkImage it_Image : l_OldImages)
        {
            m_ResourceStateTracker.ForgetImage(it_Image);
        }

        RecreateViewport();
    }

    void VulkanRenderer::RecreateViewport()
    {
        if (m_SceneViewportWidth == 0 || m_SceneViewportHeight == 0)
        {
            return;
        }

        m_RenderGraph.Recreate(m_SceneViewportWidth, m_SceneViewportHeight);

        // Re-wire cross-pass resources after recreation since image views may have changed.
        WirePassResources();
    }

    void VulkanRenderer::BuildPassContext(VulkanPassContext& outContext) const
    {
        outContext.Device = m_Device.GetDevice();
        outContext.PhysicalDevice = m_Device.GetPhysicalDevice();
        outContext.HostAllocator = m_Context.GetAllocator();
        outContext.Allocator = const_cast<VulkanAllocator*>(&m_Allocator);
        outContext.UploadContext = const_cast<VulkanUploadContext*>(&m_UploadContext);
        outContext.ResourceStateTracker = const_cast<VulkanResourceStateTracker*>(&m_ResourceStateTracker);
        outContext.Shaders = const_cast<ShaderLibrary*>(&m_ShaderLibrary);
        outContext.GraphicsQueue = m_Device.GetGraphicsQueue();
        outContext.GraphicsQueueFamilyIndex = m_Device.GetGraphicsQueueFamilyIndex();
        outContext.SwapchainFormat = m_Swapchain.GetImageFormat();
        outContext.DepthFormat = Utilities::VulkanUtilities::FindDepthFormat(m_Device.GetPhysicalDevice());
        outContext.FramesInFlight = m_FramesInFlight;
        outContext.ColorPolicy = m_Swapchain.GetSceneColorPolicy();
    }

    void VulkanRenderer::WirePassResources()
    {
        auto* l_ShadowPass = m_RenderGraph.GetPass<VulkanShadowPass>();
        auto* l_GeomPass = m_RenderGraph.GetPass<VulkanGeometryPass>();
        auto* l_LightPass = m_RenderGraph.GetPass<VulkanLightingPass>();
        auto* l_PostPass = m_RenderGraph.GetPass<VulkanPostProcessPass>();

        if (l_LightPass && l_GeomPass)
        {
            l_LightPass->SetGBufferResources(
                l_GeomPass->GetAlbedoView(),
                l_GeomPass->GetNormalView(),
                l_GeomPass->GetMaterialView(),
                l_GeomPass->GetDepthImageView()
            );
        }

        if (l_LightPass && l_ShadowPass)
        {
            l_LightPass->SetShadowResources(
                l_ShadowPass->GetShadowMapView(),
                l_ShadowPass->GetShadowMapSampler()
            );
        }

        if (l_PostPass && l_LightPass)
        {
            l_PostPass->SetSceneColorResources(
                l_LightPass->GetSceneViewportImageView(),
                l_LightPass->GetSceneViewportSampler()
            );
        }
    }

    void VulkanRenderer::InitializePasses()
    {
        VulkanPassContext l_Context{};
        BuildPassContext(l_Context);

        auto l_ShadowPass = std::make_unique<VulkanShadowPass>();
        auto l_GeomPass = std::make_unique<VulkanGeometryPass>();
        auto l_LightPass = std::make_unique<VulkanLightingPass>();
        auto l_PostPass = std::make_unique<VulkanPostProcessPass>();

        l_ShadowPass->Initialize(l_Context);
        l_GeomPass->Initialize(l_Context);
        l_LightPass->Initialize(l_Context);
        l_PostPass->Initialize(l_Context);

        m_RenderGraph.AddPass(std::move(l_ShadowPass));
        m_RenderGraph.AddPass(std::move(l_GeomPass));
        m_RenderGraph.AddPass(std::move(l_LightPass));
        m_RenderGraph.AddPass(std::move(l_PostPass));

        WirePassResources();
    }

    void VulkanRenderer::EnsurePrimitiveUploaded(Geometry::PrimitiveType primitive)
    {
        const size_t l_Index = static_cast<size_t>(primitive);
        if (l_Index >= m_Primitives.size())
        {
            TR_CORE_CRITICAL("VulkanRenderer::EnsurePrimitiveUploaded — PrimitiveType out of range");
            std::abort();
        }

        auto& a_Prim = m_Primitives[l_Index];
        if (a_Prim.VulkanVB && a_Prim.VulkanIB)
        {
            return;
        }

        const auto& a_Mesh = Geometry::GetPrimitive(primitive);

        a_Prim.VulkanVB = std::make_unique<VulkanVertexBuffer>(
            m_Allocator, m_UploadContext,
            static_cast<uint64_t>(a_Mesh.Vertices.size() * sizeof(Geometry::Vertex)),
            static_cast<uint32_t>(sizeof(Geometry::Vertex)),
            BufferMemoryUsage::GPUOnly, a_Mesh.Vertices.data()
        );

        a_Prim.VulkanIB = std::make_unique<VulkanIndexBuffer>(
            m_Allocator, m_UploadContext,
            static_cast<uint64_t>(a_Mesh.Indices.size() * sizeof(uint32_t)),
            static_cast<uint32_t>(a_Mesh.Indices.size()),
            IndexType::UInt32, BufferMemoryUsage::GPUOnly, a_Mesh.Indices.data()
        );
    }

    void VulkanRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection)
    {
        if (!m_FrameBegun || !m_ScenePassRecording)
        {
            return;
        }

        EnsurePrimitiveUploaded(primitive);

        const size_t l_Index = static_cast<size_t>(primitive);
        auto& a_Prim = m_Primitives[l_Index];
        const VkCommandBuffer l_CB = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

        VkBuffer l_VkVB = a_Prim.VulkanVB->GetVkBuffer();
        VkDeviceSize l_Offset = 0;
        vkCmdBindVertexBuffers(l_CB, 0, 1, &l_VkVB, &l_Offset);
        vkCmdBindIndexBuffer(l_CB, a_Prim.VulkanIB->GetVkBuffer(), 0, ToVkIndexType(a_Prim.VulkanIB->GetIndexType()));

        SimplePushConstants l_PC{};
        l_PC.ModelViewProjection = viewProjection * model;
        l_PC.Color = color;
        l_PC.ColorInputTransfer = static_cast<uint32_t>(m_Swapchain.GetSceneColorPolicy().SceneInputTransfer);
        l_PC.ColorOutputTransfer = static_cast<uint32_t>(m_Swapchain.GetSceneColorPolicy().SceneOutputTransfer);

        vkCmdPushConstants(l_CB, m_Pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstants), &l_PC);
        vkCmdDrawIndexed(l_CB, a_Prim.VulkanIB->GetIndexCount(), 1, 0, 0, 0);
    }

    void VulkanRenderer::DrawMesh(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection)
    {
        if (!m_FrameBegun || !m_ScenePassRecording)
        {
            return;
        }

        const VkCommandBuffer l_CB = m_Command.GetCommandBuffer(m_CurrentFrameIndex);

        VkBuffer l_VkVB = reinterpret_cast<VkBuffer>(vertexBuffer.GetNativeHandle());
        VkBuffer l_VkIB = reinterpret_cast<VkBuffer>(indexBuffer.GetNativeHandle());
        VkDeviceSize l_Offset = 0;

        vkCmdBindVertexBuffers(l_CB, 0, 1, &l_VkVB, &l_Offset);
        vkCmdBindIndexBuffer(l_CB, l_VkIB, 0, ToVkIndexType(indexBuffer.GetIndexType()));

        SimplePushConstants l_PC{};
        l_PC.ModelViewProjection = viewProjection * model;
        l_PC.Color = color;
        l_PC.ColorInputTransfer = static_cast<uint32_t>(m_Swapchain.GetSceneColorPolicy().SceneInputTransfer);
        l_PC.ColorOutputTransfer = static_cast<uint32_t>(m_Swapchain.GetSceneColorPolicy().SceneOutputTransfer);

        vkCmdPushConstants(l_CB, m_Pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstants), &l_PC);
        vkCmdDrawIndexed(l_CB, indexCount, 1, 0, 0, 0);
    }
}