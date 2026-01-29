#include "Engine/Renderer/Vulkan/VulkanRenderer.h"

#include "Engine/Utilities/Utilities.h"
#include "Engine/Platform/Window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <vector>
#include <limits>

namespace Engine
{
    struct Vertex
    {
        glm::vec2 Position;
        glm::vec3 Color;

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription l_Binding{};
            l_Binding.binding = 0;
            l_Binding.stride = sizeof(Vertex);
            l_Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return l_Binding;
        }

        static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 2> l_Attributes{};

            l_Attributes[0].binding = 0;
            l_Attributes[0].location = 0;
            l_Attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
            l_Attributes[0].offset = offsetof(Vertex, Position);

            l_Attributes[1].binding = 0;
            l_Attributes[1].location = 1;
            l_Attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            l_Attributes[1].offset = offsetof(Vertex, Color);

            return l_Attributes;
        }
    };

    static const std::array<Vertex, 3> s_TriangleVertices =
    {
        Vertex{ {  0.0f, -0.5f }, { 1.0f, 0.2f, 0.2f } },
        Vertex{ {  0.5f,  0.5f }, { 0.2f, 1.0f, 0.2f } },
        Vertex{ { -0.5f,  0.5f }, { 0.2f, 0.2f, 1.0f } },
    };

    void VulkanRenderer::Initialize(Window* window)
    {
        m_Window = window;
        m_NativeWindow = static_cast<GLFWwindow*>(window->GetNativeWindow());

        if (!m_NativeWindow)
        {
            TR_CORE_CRITICAL("VulkanRenderer: Window native handle is null (expected GLFWwindow*).");

            std::abort();
        }

        m_VulkanDevice.Initialize(m_NativeWindow);

        VulkanDevice::QueueFamilyIndices l_Indices = m_VulkanDevice.GetQueueFamilyIndices();

        VkPhysicalDevice l_PhysicalDevice = m_VulkanDevice.GetPhysicalDevice();
        VkDevice l_Device = m_VulkanDevice.GetDevice();
        VkSurfaceKHR l_Surface = m_VulkanDevice.GetSurface();

        m_Swapchain.Initialize(l_PhysicalDevice, l_Device, l_Surface, m_NativeWindow, l_Indices.GraphicsFamily.value(), l_Indices.PresentFamily.value());
        m_Swapchain.Create();

        m_Command.Initialize(l_Device, l_Indices.GraphicsFamily.value());
        m_Command.Create(s_MaxFramesInFlight);

        m_Resources.Initialize(l_PhysicalDevice, l_Device, m_VulkanDevice.GetGraphicsQueue(), m_Command.GetCommandPool());

        m_RenderPass.Initialize(l_Device);
        m_RenderPass.CreateSwapchainPass(m_Swapchain.GetImageFormat(), VK_FORMAT_UNDEFINED);

        m_Framebuffers.Initialize(l_Device);
        m_Framebuffers.SetSwapchainViews(m_Swapchain.GetImageFormat(), m_Swapchain.GetImageViews());
        VkImageView l_DepthView = VK_NULL_HANDLE;
        m_Framebuffers.Create(m_RenderPass.GetSwapchainRenderPass(), m_Swapchain.GetExtent(), l_DepthView);

        m_Descriptors.Initialize(l_Device);
        m_FrameResources.Initialize(l_Device, &m_Resources, &m_Descriptors, s_MaxFramesInFlight);
        m_FrameResources.CreateUniformBuffers();
        m_FrameResources.CreateDescriptors();

        m_Pipeline.Initialize(l_Device);
        CreatePipeline();

        m_Sync.Initialize(l_Device, s_MaxFramesInFlight);
        m_Sync.CreatePerFrame();
        m_Sync.RecreatePerImage(m_Swapchain.GetImageCount());

        CreateTriangleResources();

        TR_CORE_INFO("VulkanRenderer initialized.");
    }

    void VulkanRenderer::Shutdown()
    {
        WaitIdle();

        DestroyTriangleResources();

        CleanupSwapchain(); // destroys pipeline + renderpass; required before destroying descriptor set layouts

        m_Framebuffers.Shutdown();
        m_FrameResources.Shutdown();
        m_Swapchain.Shutdown();
        m_Sync.Shutdown();
        m_Command.Shutdown();
        m_Resources.Shutdown();
        m_Pipeline.Shutdown();
        m_Descriptors.Shutdown();
        m_RenderPass.Shutdown();
        m_VulkanDevice.Shutdown();

        TR_CORE_INFO("VulkanRenderer shutdown.");
    }

    bool VulkanRenderer::BeginFrame()
    {
        m_Sync.WaitForFrame(m_CurrentFrame);

        uint32_t l_ImageIndex = 0;
        VkResult l_Acquire = vkAcquireNextImageKHR(m_VulkanDevice.GetDevice(), m_Swapchain.GetHandle(), std::numeric_limits<uint64_t>::max(),
            m_Sync.GetImageAvailableSemaphore(m_CurrentFrame), VK_NULL_HANDLE, &l_ImageIndex);

        if (l_Acquire == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain();

            return false;
        }

        if (l_Acquire != VK_SUCCESS && l_Acquire != VK_SUBOPTIMAL_KHR)
        {
            TR_CORE_CRITICAL("vkAcquireNextImageKHR failed (VkResult = {})", static_cast<int>(l_Acquire));

            return false;
        }

        m_CurrentImageIndex = l_ImageIndex;
        m_Sync.WaitForImage(m_CurrentImageIndex);
        m_Sync.SetImageInFlight(m_CurrentImageIndex, m_Sync.GetInFlightFence(m_CurrentFrame));

        m_Sync.ResetFrame(m_CurrentFrame);
        m_Command.ResetCommandBuffer(m_CurrentFrame);

        return true;
    }

    void VulkanRenderer::Execute(const std::vector<Command>& commandList)
    {
        m_FrameResources.UpdateUniformBuffer(m_CurrentFrame, m_Swapchain.GetExtent());
        RecordCommandBuffer(m_Command.GetCommandBuffer(m_CurrentFrame), m_CurrentImageIndex, commandList);
    }

    void VulkanRenderer::EndFrame()
    {
        VkSemaphore l_WaitSemaphores[] = { m_Sync.GetImageAvailableSemaphore(m_CurrentFrame) };
        VkPipelineStageFlags l_WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore l_SignalSemaphores[] = { m_Sync.GetRenderFinishedSemaphore(m_CurrentImageIndex) };

        VkCommandBuffer l_Cmd = m_Command.GetCommandBuffer(m_CurrentFrame);

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.waitSemaphoreCount = 1;
        l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores;
        l_SubmitInfo.pWaitDstStageMask = l_WaitStages;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &l_Cmd;
        l_SubmitInfo.signalSemaphoreCount = 1;
        l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkQueueSubmit(m_VulkanDevice.GetGraphicsQueue(), 1, &l_SubmitInfo, m_Sync.GetInFlightFence(m_CurrentFrame)), "vkQueueSubmit");

        VkSwapchainKHR l_Swapchains[] = { m_Swapchain.GetHandle() };

        VkPresentInfoKHR l_PresentInfo{};
        l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        l_PresentInfo.waitSemaphoreCount = 1;
        l_PresentInfo.pWaitSemaphores = l_SignalSemaphores;
        l_PresentInfo.swapchainCount = 1;
        l_PresentInfo.pSwapchains = l_Swapchains;
        l_PresentInfo.pImageIndices = &m_CurrentImageIndex;

        VkResult l_Present = vkQueuePresentKHR(m_VulkanDevice.GetPresentQueue(), &l_PresentInfo);

        if (l_Present == VK_ERROR_OUT_OF_DATE_KHR || l_Present == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
        {
            m_FramebufferResized = false;
            RecreateSwapchain();
        }
        else if (l_Present != VK_SUCCESS)
        {
            Engine::Utilities::VulkanUtilities::VKCheckStrict(l_Present, "vkQueuePresentKHR");
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % s_MaxFramesInFlight;
    }

    void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
    {
        (void)width;
        (void)height;

        m_FramebufferResized = true;
    }

    void VulkanRenderer::WaitIdle()
    {
        VkDevice l_Device = m_VulkanDevice.GetDevice();
        if (l_Device)
        {
            vkDeviceWaitIdle(l_Device);
        }
    }

    void VulkanRenderer::CreatePipeline()
    {
        const auto l_Binding = Vertex::GetBindingDescription();
        const auto l_AttrArr = Vertex::GetAttributeDescriptions();

        std::vector<VkVertexInputAttributeDescription> l_Attributes;
        l_Attributes.reserve(l_AttrArr.size());
        for (const auto& it_Attr : l_AttrArr)
        {
            l_Attributes.push_back(it_Attr);
        }

        std::vector<VkDescriptorSetLayout> l_SetLayouts =
        {
            m_Descriptors.GetLayout()
        };

        m_Pipeline.CreateGraphicsPipeline(m_RenderPass.GetSwapchainRenderPass(), l_Binding, l_Attributes, "Assets/Shaders/Simple.vert.spv", "Assets/Shaders/Simple.frag.spv", l_SetLayouts);
    }

    void VulkanRenderer::CreateTriangleResources()
    {
        const VkDeviceSize l_SizeBytes = static_cast<VkDeviceSize>(sizeof(s_TriangleVertices));
        m_Resources.CreateVertexBufferStaged(s_TriangleVertices.data(), l_SizeBytes, m_VertexBuffer, m_VertexBufferMemory);
    }

    void VulkanRenderer::DestroyTriangleResources()
    {
        m_Resources.DestroyBuffer(m_VertexBuffer, m_VertexBufferMemory);
    }

    void VulkanRenderer::CleanupSwapchain()
    {
        m_Sync.DestroyPerImage();

        m_Framebuffers.Cleanup();
        m_Pipeline.Cleanup();
        m_RenderPass.CleanupSwapchainPass();

        m_Swapchain.Cleanup();
    }

    void VulkanRenderer::RecreateSwapchain()
    {
        int l_Width = 0;
        int l_Height = 0;
        glfwGetFramebufferSize(m_NativeWindow, &l_Width, &l_Height);

        while (l_Width == 0 || l_Height == 0)
        {
            glfwGetFramebufferSize(m_NativeWindow, &l_Width, &l_Height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_VulkanDevice.GetDevice());

        CleanupSwapchain();

        m_Swapchain.Create();
        m_RenderPass.CreateSwapchainPass(m_Swapchain.GetImageFormat(), VK_FORMAT_UNDEFINED);
        CreatePipeline();
        m_Framebuffers.SetSwapchainViews(m_Swapchain.GetImageFormat(), m_Swapchain.GetImageViews());
        VkImageView l_DepthView = VK_NULL_HANDLE;
        m_Framebuffers.Create(m_RenderPass.GetSwapchainRenderPass(), m_Swapchain.GetExtent(), l_DepthView);

        m_Sync.RecreatePerImage(m_Swapchain.GetImageCount());

        TR_CORE_INFO("Swapchain recreated: {}x{}", m_Swapchain.GetExtent().width, m_Swapchain.GetExtent().height);
    }

    void VulkanRenderer::RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, const std::vector<Command>& commandList)
    {
        VkCommandBufferBeginInfo l_Begin{};
        l_Begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkBeginCommandBuffer(cmd, &l_Begin), "vkBeginCommandBuffer");

        for (const auto& it_Cmd : commandList)
        {
            if (it_Cmd.Type == CommandType::Clear)
            {
                m_LastClearColor = it_Cmd.Color;
            }
        }

        VkClearValue l_Clear{};
        l_Clear.color = { { m_LastClearColor.r, m_LastClearColor.g, m_LastClearColor.b, m_LastClearColor.a } };

        const auto& a_Framebuffers = m_Framebuffers.GetFramebuffers();
        VkRenderPassBeginInfo l_RP{};
        l_RP.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        l_RP.renderPass = m_RenderPass.GetSwapchainRenderPass();
        l_RP.framebuffer = a_Framebuffers[imageIndex];
        l_RP.renderArea.offset = { 0, 0 };
        l_RP.renderArea.extent = m_Swapchain.GetExtent();
        l_RP.clearValueCount = 1;
        l_RP.pClearValues = &l_Clear;

        vkCmdBeginRenderPass(cmd, &l_RP, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport l_Viewport{};
        l_Viewport.x = 0.0f;
        l_Viewport.y = 0.0f;
        l_Viewport.width = (float)m_Swapchain.GetExtent().width;
        l_Viewport.height = (float)m_Swapchain.GetExtent().height;
        l_Viewport.minDepth = 0.0f;
        l_Viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &l_Viewport);

        VkRect2D l_Scissor{};
        l_Scissor.offset = { 0, 0 };
        l_Scissor.extent = m_Swapchain.GetExtent();
        vkCmdSetScissor(cmd, 0, 1, &l_Scissor);

        for (const auto& it_Cmd : commandList)
        {
            if (it_Cmd.Type == CommandType::DrawTriangle)
            {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.GetHandle());

                VkDescriptorSet l_Set = m_FrameResources.GetDescriptorSet(m_CurrentFrame);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.GetLayout(), 0, 1, &l_Set, 0, nullptr);

                VkBuffer l_VB[] = { m_VertexBuffer };
                VkDeviceSize l_Offsets[] = { 0 };
                vkCmdBindVertexBuffers(cmd, 0, 1, l_VB, l_Offsets);

                vkCmdDraw(cmd, 3, 1, 0, 0);
            }
        }

        vkCmdEndRenderPass(cmd);

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");
    }
}