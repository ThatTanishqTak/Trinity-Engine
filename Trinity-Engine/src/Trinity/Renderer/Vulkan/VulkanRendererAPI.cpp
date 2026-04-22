#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanFramebuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanShader.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanSampler.h"
#include "Trinity/Renderer/Vulkan/RenderGraph/VulkanRenderGraph.h"
#include "Trinity/Platform/Window/Window.h"
#include "Trinity/Utilities/Log.h"

#include <stb_image.h>
#include <cstring>
#include <limits>

namespace Trinity
{
    VulkanRendererAPI::VulkanRendererAPI()
    {
        m_Backend = RendererBackend::Vulkan;
    }

    VulkanRendererAPI::~VulkanRendererAPI()
    {

    }

    void VulkanRendererAPI::Initialize(Window& window, const RendererAPISpecification& specification)
    {
        m_MaxFramesInFlight = specification.MaxFramesInFlight;

        m_Instance.Initialize(window, specification.EnableValidation);
        m_Device.Initialize(m_Instance, specification.EnableValidation);

        if (m_Instance.IsValidationEnabled())
        {
            m_Debug.Initialize(m_Instance.GetInstance());
        }

        m_Allocator.Initialize(m_Device);
        m_Swapchain.Initialize(m_Device, window.GetWidth(), window.GetHeight(), true);
        m_CommandPool.Initialize(m_Device, m_MaxFramesInFlight);
        m_SyncObjects.Initialize(m_Device.GetDevice(), m_MaxFramesInFlight, m_Swapchain.GetImageCount());

        TR_CORE_INFO("Vulkan renderer initialized");
    }

    void VulkanRendererAPI::Shutdown()
    {
        vkDeviceWaitIdle(m_Device.GetDevice());

        m_SyncObjects.Shutdown();
        m_CommandPool.Shutdown();
        m_Swapchain.Shutdown();
        m_Allocator.Shutdown();
        m_Debug.Shutdown();
        m_Device.Shutdown();
        m_Instance.Shutdown();

        TR_CORE_INFO("Vulkan renderer shut down");
    }

    bool VulkanRendererAPI::BeginFrame()
    {
        m_SyncObjects.WaitForFence(m_CurrentFrameIndex);

        VkResult l_Result = vkAcquireNextImageKHR(m_Device.GetDevice(), m_Swapchain.GetSwapchain(), std::numeric_limits<uint64_t>::max(), m_SyncObjects.GetImageAvailableSemaphore(m_CurrentFrameIndex), VK_NULL_HANDLE, &m_CurrentImageIndex);

        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_FramebufferResized = false;
            m_Swapchain.Recreate(m_Swapchain.GetExtent().width, m_Swapchain.GetExtent().height);

            return false;
        }

        if (l_Result != VK_SUCCESS && l_Result != VK_SUBOPTIMAL_KHR)
        {
            TR_CORE_ERROR("Failed to acquire swapchain image.");
            return false;
        }

        m_SyncObjects.ResetFence(m_CurrentFrameIndex);
        m_CommandPool.ResetCommandBuffer(m_CurrentFrameIndex);
        m_CommandPool.BeginCommandBuffer(m_CurrentFrameIndex);

        VkCommandBuffer l_CommandBuffer = GetCurrentCommandBuffer();

        TransitionImageLayout(l_CommandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        m_FrameStarted = true;

        return true;
    }

    void VulkanRendererAPI::EndFrame()
    {
        if (!m_FrameStarted)
        {
            return;
        }

        VkCommandBuffer l_CommandBuffer = GetCurrentCommandBuffer();

        TransitionImageLayout(l_CommandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        m_CommandPool.EndCommandBuffer(m_CurrentFrameIndex);

        m_FrameStarted = false;
    }

    void VulkanRendererAPI::Present()
    {
        VkSemaphore l_WaitSemaphores[] = { m_SyncObjects.GetImageAvailableSemaphore(m_CurrentFrameIndex) };
        VkSemaphore l_SignalSemaphores[] = { m_SyncObjects.GetRenderFinishedSemaphore(m_CurrentImageIndex) };
        VkPipelineStageFlags l_WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkCommandBuffer l_CommandBuffer = m_CommandPool.GetCommandBuffer(m_CurrentFrameIndex);

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.waitSemaphoreCount = 1;
        l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores;
        l_SubmitInfo.pWaitDstStageMask = l_WaitStages;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;
        l_SubmitInfo.signalSemaphoreCount = 1;
        l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores;

        VulkanUtilities::VKCheck(vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &l_SubmitInfo, m_SyncObjects.GetInFlightFence(m_CurrentFrameIndex)), "Failed vkQueueSubmit");

        VkSwapchainKHR l_Swapchains[] = { m_Swapchain.GetSwapchain() };

        VkPresentInfoKHR l_PresentInfo{};
        l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        l_PresentInfo.waitSemaphoreCount = 1;
        l_PresentInfo.pWaitSemaphores = l_SignalSemaphores;
        l_PresentInfo.swapchainCount = 1;
        l_PresentInfo.pSwapchains = l_Swapchains;
        l_PresentInfo.pImageIndices = &m_CurrentImageIndex;

        VkResult l_Result = vkQueuePresentKHR(m_Device.GetPresentQueue(), &l_PresentInfo);

        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR || l_Result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
        {
            m_FramebufferResized = false;
            m_Swapchain.Recreate(m_Swapchain.GetExtent().width, m_Swapchain.GetExtent().height);
        }
        else if (l_Result != VK_SUCCESS)
        {
            TR_CORE_ERROR("Failed to present swapchain image.");
        }

        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFramesInFlight;
    }

    void VulkanRendererAPI::WaitIdle()
    {
        vkDeviceWaitIdle(m_Device.GetDevice());
    }

    void VulkanRendererAPI::OnWindowResize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        m_FramebufferResized = true;
    }

    uint32_t VulkanRendererAPI::GetSwapchainWidth() const
    {
        return m_Swapchain.GetExtent().width;
    }

    uint32_t VulkanRendererAPI::GetSwapchainHeight() const
    {
        return m_Swapchain.GetExtent().height;
    }

    VkCommandBuffer VulkanRendererAPI::GetCurrentCommandBuffer() const
    {
        return m_CommandPool.GetCommandBuffer(m_CurrentFrameIndex);
    }

    std::shared_ptr<Buffer> VulkanRendererAPI::CreateBuffer(const BufferSpecification& specification)
    {
        return std::make_shared<VulkanBuffer>(m_Device.GetDevice(), m_Allocator.GetAllocator(), specification);
    }

    std::shared_ptr<Texture> VulkanRendererAPI::CreateTexture(const TextureSpecification& specification)
    {
        return std::make_shared<VulkanTexture>(m_Device.GetDevice(), m_Allocator.GetAllocator(), specification);
    }

    std::shared_ptr<Texture> VulkanRendererAPI::CreateTextureFromData(const void* data, uint32_t width, uint32_t height)
    {
        VkDeviceSize l_ImageSize = static_cast<VkDeviceSize>(width) * height * 4;

        // Staging buffer (CPU-visible)
        VkBufferCreateInfo l_BufferCreateInfo{};
        l_BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        l_BufferCreateInfo.size = l_ImageSize;
        l_BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        l_BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo l_BufferAllocateInfo{};
        l_BufferAllocateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        l_BufferAllocateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer l_StagingBuffer = VK_NULL_HANDLE;
        VmaAllocation l_StagingAllocation = VK_NULL_HANDLE;
        VmaAllocationInfo l_StagingInfo{};
        VulkanUtilities::VKCheck(vmaCreateBuffer(m_Allocator.GetAllocator(), &l_BufferCreateInfo, &l_BufferAllocateInfo, &l_StagingBuffer, &l_StagingAllocation, &l_StagingInfo), "CreateTextureFromData: failed to create staging buffer");

        std::memcpy(l_StagingInfo.pMappedData, data, static_cast<size_t>(l_ImageSize));
        vmaFlushAllocation(m_Allocator.GetAllocator(), l_StagingAllocation, 0, l_ImageSize);

        // GPU texture (needs TransferDestination usage)
        TextureSpecification l_TextureSpecification;
        l_TextureSpecification.Width = width;
        l_TextureSpecification.Height = height;
        l_TextureSpecification.Format = TextureFormat::RGBA8;
        l_TextureSpecification.Usage = TextureUsage::Sampled | TextureUsage::TransferDestination;

        auto a_Texture = std::make_shared<VulkanTexture>(m_Device.GetDevice(), m_Allocator.GetAllocator(), l_TextureSpecification);

        VkCommandBuffer l_CommandBuffer = m_CommandPool.BeginSingleTimeCommands();

        // UNDEFINED -> TRANSFER_DST
        {
            VkImageMemoryBarrier l_Barrier{};
            l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            l_Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            l_Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            l_Barrier.image = a_Texture->GetImage();
            l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(l_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &l_Barrier);
        }

        // Buffer -> Image copy
        {
            VkBufferImageCopy l_Region{};
            l_Region.bufferOffset = 0;
            l_Region.bufferRowLength = 0;
            l_Region.bufferImageHeight = 0;
            l_Region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
            l_Region.imageOffset = { 0, 0, 0 };
            l_Region.imageExtent = { width, height, 1 };

            vkCmdCopyBufferToImage(l_CommandBuffer, l_StagingBuffer, a_Texture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_Region);
        }

        // TRANSFER_DST -> SHADER_READ_ONLY
        {
            VkImageMemoryBarrier l_Barrier{};
            l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            l_Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            l_Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            l_Barrier.image = a_Texture->GetImage();
            l_Barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            l_Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(l_CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &l_Barrier);
        }

        m_CommandPool.EndSingleTimeCommands(l_CommandBuffer);

        vmaDestroyBuffer(m_Allocator.GetAllocator(), l_StagingBuffer, l_StagingAllocation);

        return a_Texture;
    }

    std::shared_ptr<Texture> VulkanRendererAPI::LoadTextureFromFile(const std::string& path)
    {
        int l_Width = 0, l_Height = 0, l_Channels = 0;
        stbi_uc* l_Pixels = stbi_load(path.c_str(), &l_Width, &l_Height, &l_Channels, STBI_rgb_alpha);

        if (!l_Pixels)
        {
            TR_CORE_WARN("LoadTextureFromFile: failed to load '{}'", path);
            return nullptr;
        }

        auto a_Texture = CreateTextureFromData(l_Pixels, static_cast<uint32_t>(l_Width), static_cast<uint32_t>(l_Height));
        stbi_image_free(l_Pixels);

        return a_Texture;
    }

    std::shared_ptr<Texture> VulkanRendererAPI::CreateTextureFromMemory(const uint8_t* data, size_t size)
    {
        int l_Width = 0;
        int l_Height = 0;
        int l_Channels = 0;

        stbi_uc* l_Pixels = stbi_load_from_memory(data, static_cast<int>(size), &l_Width, &l_Height, &l_Channels, STBI_rgb_alpha);

        if (!l_Pixels)
        {
            TR_CORE_WARN("CreateTextureFromMemory: failed to decode image data");
            return nullptr;
        }

        auto a_Texture = CreateTextureFromData(l_Pixels, static_cast<uint32_t>(l_Width), static_cast<uint32_t>(l_Height));
        stbi_image_free(l_Pixels);

        return a_Texture;
    }

    std::shared_ptr<Framebuffer> VulkanRendererAPI::CreateFramebuffer(const FramebufferSpecification& specification)
    {
        return std::make_shared<VulkanFramebuffer>(m_Device.GetDevice(), m_Allocator.GetAllocator(), specification);
    }

    std::shared_ptr<Shader> VulkanRendererAPI::CreateShader(const ShaderSpecification& specification)
    {
        return std::make_shared<VulkanShader>(m_Device.GetDevice(), specification);
    }

    std::shared_ptr<Pipeline> VulkanRendererAPI::CreatePipeline(const PipelineSpecification& specification)
    {
        return std::make_shared<VulkanPipeline>(m_Device.GetDevice(), specification);
    }

    std::shared_ptr<Sampler> VulkanRendererAPI::CreateSampler(const SamplerSpecification& specification)
    {
        return std::make_shared<VulkanSampler>(m_Device.GetDevice(), specification);
    }

    std::unique_ptr<RenderGraph> VulkanRendererAPI::CreateRenderGraph()
    {
        return std::make_unique<VulkanRenderGraph>(*this);
    }

    void VulkanRendererAPI::TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) const
    {
        VkImageMemoryBarrier2 l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        l_Barrier.oldLayout = oldLayout;
        l_Barrier.newLayout = newLayout;
        l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.image = image;
        l_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        l_Barrier.subresourceRange.baseMipLevel = 0;
        l_Barrier.subresourceRange.levelCount = 1;
        l_Barrier.subresourceRange.baseArrayLayer = 0;
        l_Barrier.subresourceRange.layerCount = 1;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            l_Barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            l_Barrier.dstAccessMask = 0;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            l_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            l_Barrier.srcAccessMask = 0;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            l_Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        }
        else
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            l_Barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
            l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            l_Barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        }

        VkDependencyInfo l_DependencyInfo{};
        l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_DependencyInfo.imageMemoryBarrierCount = 1;
        l_DependencyInfo.pImageMemoryBarriers = &l_Barrier;

        vkCmdPipelineBarrier2(cmd, &l_DependencyInfo);
    }
}