#include "Engine/Renderer/Vulkan/VulkanRenderer.h"

#include "Engine/Utilities/Utilities.h"
#include "Engine/Platform/Window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <string>
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

        CreateRenderPass();

        m_Descriptors.Initialize(l_Device);
        CreateUniformBuffers();
        CreateDescriptors();

        m_Pipeline.Initialize(l_Device);
        CreatePipeline();

        CreateFramebuffers();

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

        DestroyUniformBuffers();

        CleanupSwapchain(); // destroys pipeline + renderpass; required before destroying descriptor set layouts

        DestroyDescriptors();

        m_Swapchain.Shutdown();

        m_Sync.Shutdown();
        m_Command.Shutdown();
        m_Resources.Shutdown();
        m_Pipeline.Shutdown();
        m_Descriptors.Shutdown();
        m_VulkanDevice.Shutdown();

        TR_CORE_INFO("VulkanRenderer shutdown.");
    }

    void VulkanRenderer::BeginFrame()
    {
        m_Sync.WaitForFrame(m_CurrentFrame);

        VkResult l_Acquire = vkAcquireNextImageKHR(m_VulkanDevice.GetDevice(), m_Swapchain.GetHandle(), std::numeric_limits<uint64_t>::max(), 
            m_Sync.GetImageAvailableSemaphore(m_CurrentFrame), VK_NULL_HANDLE, &m_CurrentImageIndex);

        if (l_Acquire == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain();

            return;
        }

        if (l_Acquire != VK_SUCCESS && l_Acquire != VK_SUBOPTIMAL_KHR)
        {
            Engine::Utilities::VulkanUtilities::VKCheckStrict(l_Acquire, "vkAcquireNextImageKHR");
        }

        m_Sync.WaitForImage(m_CurrentImageIndex);
        m_Sync.SetImageInFlight(m_CurrentImageIndex, m_Sync.GetInFlightFence(m_CurrentFrame));

        m_Sync.ResetFrame(m_CurrentFrame);
        m_Command.ResetCommandBuffer(m_CurrentFrame);
    }

    void VulkanRenderer::Execute(const std::vector<Command>& commandList)
    {
        UpdateUniformBuffer(m_CurrentFrame);
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

    void VulkanRenderer::CreateRenderPass()
    {
        VkAttachmentDescription l_ColorAttachment{};
        l_ColorAttachment.format = m_Swapchain.GetImageFormat();
        l_ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        l_ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        l_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        l_ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        l_ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        l_ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        l_ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference l_ColorRef{};
        l_ColorRef.attachment = 0;
        l_ColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription l_Subpass{};
        l_Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        l_Subpass.colorAttachmentCount = 1;
        l_Subpass.pColorAttachments = &l_ColorRef;

        VkSubpassDependency l_Dep{};
        l_Dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        l_Dep.dstSubpass = 0;
        l_Dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        l_Dep.srcAccessMask = 0;
        l_Dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        l_Dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo l_RPInfo{};
        l_RPInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        l_RPInfo.attachmentCount = 1;
        l_RPInfo.pAttachments = &l_ColorAttachment;
        l_RPInfo.subpassCount = 1;
        l_RPInfo.pSubpasses = &l_Subpass;
        l_RPInfo.dependencyCount = 1;
        l_RPInfo.pDependencies = &l_Dep;

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateRenderPass(m_VulkanDevice.GetDevice(), &l_RPInfo, nullptr, &m_RenderPass), "vkCreateRenderPass");
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

        m_Pipeline.CreateGraphicsPipeline(m_RenderPass, l_Binding, l_Attributes, "Assets/Shaders/Simple.vert.spv", "Assets/Shaders/Simple.frag.spv", l_SetLayouts);
    }

    void VulkanRenderer::CreateFramebuffers()
    {
        const auto& a_ImageViews = m_Swapchain.GetImageViews();
        m_Framebuffers.resize(a_ImageViews.size());

        for (size_t i = 0; i < a_ImageViews.size(); ++i)
        {
            VkImageView l_Attachments[] = { a_ImageViews[i] };

            VkFramebufferCreateInfo l_FBInfo{};
            l_FBInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            l_FBInfo.renderPass = m_RenderPass;
            l_FBInfo.attachmentCount = 1;
            l_FBInfo.pAttachments = l_Attachments;
            l_FBInfo.width = m_Swapchain.GetExtent().width;
            l_FBInfo.height = m_Swapchain.GetExtent().height;
            l_FBInfo.layers = 1;

            Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateFramebuffer(m_VulkanDevice.GetDevice(), &l_FBInfo, nullptr, &m_Framebuffers[i]), "vkCreateFramebuffer");
        }
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

    void VulkanRenderer::CreateUniformBuffers()
    {
        m_UniformBuffers.resize(s_MaxFramesInFlight);
        m_UniformBuffersMemory.resize(s_MaxFramesInFlight);
        m_UniformBuffersMapped.resize(s_MaxFramesInFlight);

        VkDeviceSize l_BufferSize = sizeof(UniformBufferObject);

        for (uint32_t i = 0; i < s_MaxFramesInFlight; ++i)
        {
            m_Resources.CreateBuffer(l_BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_UniformBuffers[i], m_UniformBuffersMemory[i]);

            void* l_Mapped = nullptr;
            Engine::Utilities::VulkanUtilities::VKCheckStrict(vkMapMemory(m_VulkanDevice.GetDevice(), m_UniformBuffersMemory[i], 0, l_BufferSize, 0, &l_Mapped), "vkMapMemory(UniformBuffer)");

            m_UniformBuffersMapped[i] = l_Mapped;
        }
    }

    void VulkanRenderer::DestroyUniformBuffers()
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_UniformBuffers.size()); ++i)
        {
            if (m_UniformBuffersMapped[i])
            {
                vkUnmapMemory(m_VulkanDevice.GetDevice(), m_UniformBuffersMemory[i]);
                m_UniformBuffersMapped[i] = nullptr;
            }

            m_Resources.DestroyBuffer(m_UniformBuffers[i], m_UniformBuffersMemory[i]);
        }

        m_UniformBuffers.clear();
        m_UniformBuffersMemory.clear();
        m_UniformBuffersMapped.clear();
    }

    void VulkanRenderer::UpdateUniformBuffer(uint32_t frameIndex)
    {
        UniformBufferObject l_Ubo{};

        float l_Aspect = 1.0f;
        VkExtent2D l_Extent = m_Swapchain.GetExtent();
        if (l_Extent.height != 0)
        {
            l_Aspect = static_cast<float>(l_Extent.width) / static_cast<float>(l_Extent.height);
        }

        glm::mat4 l_Model = glm::mat4(1.0f);
        glm::mat4 l_View = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 l_Proj = glm::perspective(glm::radians(45.0f), l_Aspect, 0.1f, 10.0f);

        // Vulkan clip space has inverted Y compared to OpenGL.
        l_Proj[1][1] *= -1.0f;

        l_Ubo.MVP = l_Proj * l_View * l_Model;

        std::memcpy(m_UniformBuffersMapped[frameIndex], &l_Ubo, sizeof(UniformBufferObject));
    }

    void VulkanRenderer::CreateDescriptors()
    {
        // Layout: binding 0 = uniform buffer (vertex shader)
        VkDescriptorSetLayoutBinding l_UboBinding{};
        l_UboBinding.binding = 0;
        l_UboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_UboBinding.descriptorCount = 1;
        l_UboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        l_UboBinding.pImmutableSamplers = nullptr;

        m_Descriptors.CreateLayout({ l_UboBinding });

        // Pool: enough UBO descriptors for frames-in-flight
        VkDescriptorPoolSize l_PoolSize{};
        l_PoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_PoolSize.descriptorCount = s_MaxFramesInFlight;

        m_Descriptors.CreatePool(s_MaxFramesInFlight, { l_PoolSize });

        m_Descriptors.AllocateSets(s_MaxFramesInFlight, m_DescriptorSets);

        for (uint32_t i = 0; i < s_MaxFramesInFlight; ++i)
        {
            m_Descriptors.UpdateBuffer(m_DescriptorSets[i], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_UniformBuffers[i], sizeof(UniformBufferObject), 0);
        }
    }

    void VulkanRenderer::DestroyDescriptors()
    {
        m_DescriptorSets.clear();
        m_Descriptors.Shutdown();
        m_Descriptors.Initialize(m_VulkanDevice.GetDevice());
        m_Descriptors.DestroyLayout();      // no-op after Shutdown, but explicit is fine
        m_Descriptors.DestroyPool();        // ditto
    }

    void VulkanRenderer::CleanupSwapchain()
    {
        m_Sync.DestroyPerImage();

        for (VkFramebuffer it_FB : m_Framebuffers)
        {
            vkDestroyFramebuffer(m_VulkanDevice.GetDevice(), it_FB, nullptr);
        }
        m_Framebuffers.clear();

        m_Pipeline.Cleanup();

        if (m_RenderPass)
        {
            vkDestroyRenderPass(m_VulkanDevice.GetDevice(), m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }

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
        CreateRenderPass();
        CreatePipeline();
        CreateFramebuffers();

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

        VkRenderPassBeginInfo l_RP{};
        l_RP.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        l_RP.renderPass = m_RenderPass;
        l_RP.framebuffer = m_Framebuffers[imageIndex];
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

                VkDescriptorSet l_Set = m_DescriptorSets[m_CurrentFrame];
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