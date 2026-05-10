#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanFramebuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanShader.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanSampler.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanDescriptorSet.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanDescriptorSetLayout.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanComputePipeline.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanQueryPool.h"
#include "Trinity/Renderer/Vulkan/RenderGraph/VulkanRenderGraph.h"
#include "Trinity/Platform/Window/Window.h"
#include "Trinity/Utilities/Log.h"

#include <stb_image.h>

#include <array>
#include <cstring>
#include <limits>

namespace Trinity
{
    namespace
    {
        void EmitSwapchainTransition(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
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
            else
            {
                l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                l_Barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
                l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                l_Barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            }

            VkDependencyInfo l_DepInfo{};
            l_DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            l_DepInfo.imageMemoryBarrierCount = 1;
            l_DepInfo.pImageMemoryBarriers = &l_Barrier;

            vkCmdPipelineBarrier2(commandBuffer, &l_DepInfo);
        }
    }

    VulkanRendererAPI::VulkanRendererAPI()
    {
        m_Backend = RendererBackend::Vulkan;
    }

    VulkanRendererAPI::~VulkanRendererAPI()
    {

    }

    void VulkanRendererAPI::Initialize(Window& window, const RendererAPISpecification& specification)
    {
        TR_CORE_INFO("INITIALIZING VULKAN");

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
        m_DescriptorAllocator.Initialize(m_Device.GetDevice(), m_MaxFramesInFlight);
        m_SyncObjects.Initialize(m_Device.GetDevice(), m_MaxFramesInFlight, m_Swapchain.GetImageCount());

        UploadQueueSpecification l_UploadSpec{};
        l_UploadSpec.TransferQueue = m_Device.GetTransferQueue();
        l_UploadSpec.ConsumerQueue = m_Device.GetGraphicsQueue();
        l_UploadSpec.TransferQueueFamily = m_Device.GetTransferQueueFamily();
        l_UploadSpec.ConsumerQueueFamily = m_Device.GetGraphicsQueueFamily();
        l_UploadSpec.HasDedicatedTransfer = m_Device.HasDedicatedTransferQueue();
        l_UploadSpec.HasTimelineSemaphore = m_Device.HasTimelineSemaphore();
        m_UploadQueue.Initialize(m_Device.GetDevice(), m_Allocator.GetAllocator(), l_UploadSpec);

        m_SupportsAsyncCompute = m_Device.HasDedicatedComputeQueue();

        TR_CORE_INFO("VULKAN INITIALIZED");
    }

    void VulkanRendererAPI::Shutdown()
    {
        TR_CORE_INFO("SHUTTING DOWN VULKAN");

        m_SamplerCache.clear();

        WaitIdle();

        m_UploadQueue.Shutdown();
        m_SyncObjects.Shutdown();
        m_LayoutCache.clear();
        m_DescriptorAllocator.Shutdown();
        m_CommandPool.Shutdown();
        m_Swapchain.Shutdown();
        m_Allocator.Shutdown();
        m_Debug.Shutdown();
        m_Device.Shutdown();
        m_Instance.Shutdown();

        TR_CORE_INFO("VULKAN SHUTDOWN COMPLETE");
    }

    bool VulkanRendererAPI::BeginFrame()
    {
        m_SyncObjects.WaitForFence(m_CurrentFrameIndex);

        m_UploadQueue.BeginFrame();
        m_UploadQueue.FlushAsync();

        VkResult l_Result = vkAcquireNextImageKHR(m_Device.GetDevice(), m_Swapchain.GetSwapchain(), std::numeric_limits<uint64_t>::max(), m_SyncObjects.GetImageAvailableSemaphore(m_CurrentFrameIndex), VK_NULL_HANDLE, &m_CurrentImageIndex);

        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_FramebufferResized = false;
            m_Swapchain.Recreate(m_Swapchain.GetExtent().width, m_Swapchain.GetExtent().height);

            return false;
        }

        if (l_Result != VK_SUCCESS && l_Result != VK_SUBOPTIMAL_KHR)
        {
            return false;
        }

        m_DescriptorAllocator.BeginFrame(m_CurrentFrameIndex);

        m_SyncObjects.ResetFence(m_CurrentFrameIndex);
        m_CommandPool.ResetCommandBuffer(m_CurrentFrameIndex);
        m_CommandPool.BeginCommandBuffer(m_CurrentFrameIndex);

        VkCommandBuffer l_CommandBuffer = GetCurrentCommandBuffer();

        m_UploadQueue.RecordAcquireBarriers(l_CommandBuffer);

        EmitSwapchainTransition(l_CommandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

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

        EmitSwapchainTransition(l_CommandBuffer, m_Swapchain.GetImages()[m_CurrentImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        m_CommandPool.EndCommandBuffer(m_CurrentFrameIndex);

        m_FrameStarted = false;
    }

    void VulkanRendererAPI::Present()
    {
        const VkSemaphore l_ImageAvailable = m_SyncObjects.GetImageAvailableSemaphore(m_CurrentFrameIndex);
        const VkSemaphore l_RenderFinished = m_SyncObjects.GetRenderFinishedSemaphore(m_CurrentImageIndex);
        VkCommandBuffer l_CommandBuffer = m_CommandPool.GetCommandBuffer(m_CurrentFrameIndex);

        const bool l_HasUploadWait = m_UploadQueue.HasPendingWaitInfo();

        std::array<VkSemaphore, 2> l_WaitSemaphores{};
        std::array<uint64_t, 2> l_WaitValues{};
        std::array<VkPipelineStageFlags, 2> l_WaitStages{};
        uint32_t l_WaitCount = 0;

        l_WaitSemaphores[l_WaitCount] = l_ImageAvailable;
        l_WaitValues[l_WaitCount] = 0;
        l_WaitStages[l_WaitCount] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        ++l_WaitCount;

        if (l_HasUploadWait)
        {
            l_WaitSemaphores[l_WaitCount] = m_UploadQueue.GetTimelineSemaphore();
            l_WaitValues[l_WaitCount] = m_UploadQueue.GetLastSubmittedValue();
            l_WaitStages[l_WaitCount] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            ++l_WaitCount;
        }

        VkTimelineSemaphoreSubmitInfo l_TimelineInfo{};
        l_TimelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        l_TimelineInfo.waitSemaphoreValueCount = l_WaitCount;
        l_TimelineInfo.pWaitSemaphoreValues = l_WaitValues.data();

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.waitSemaphoreCount = l_WaitCount;
        l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores.data();
        l_SubmitInfo.pWaitDstStageMask = l_WaitStages.data();
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;
        l_SubmitInfo.signalSemaphoreCount = 1;
        l_SubmitInfo.pSignalSemaphores = &l_RenderFinished;

        if (l_HasUploadWait)
        {
            l_SubmitInfo.pNext = &l_TimelineInfo;
        }

        VulkanUtilities::VKCheck(vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &l_SubmitInfo, m_SyncObjects.GetInFlightFence(m_CurrentFrameIndex)), "Failed vkQueueSubmit");

        VkSwapchainKHR l_Swapchains[] = { m_Swapchain.GetSwapchain() };

        VkPresentInfoKHR l_PresentInfo{};
        l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        l_PresentInfo.waitSemaphoreCount = 1;
        l_PresentInfo.pWaitSemaphores = &l_RenderFinished;
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
            TR_CORE_ERROR("Failed to present swapchain image");
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

    CommandList& VulkanRendererAPI::GetCommandList()
    {
        return m_CommandPool.GetCommandList(m_CurrentFrameIndex);
    }

    std::shared_ptr<Buffer> VulkanRendererAPI::CreateBuffer(const BufferSpecification& specification)
    {
        TR_CORE_TRACE("Creating Vulkan Buffer '{}' ({} bytes, memoryType={})", specification.DebugName, specification.Size, static_cast<int>(specification.MemoryType));

        return std::make_shared<VulkanBuffer>(m_Device.GetDevice(), m_Allocator.GetAllocator(), specification, &m_UploadQueue);
    }

    std::shared_ptr<Texture> VulkanRendererAPI::CreateTexture(const TextureSpecification& specification)
    {
        TR_CORE_TRACE("Creating Vulkan Texture '{}' ({}x{}x{}, mips={}, layers={})", specification.DebugName, specification.Width, specification.Height, specification.Depth, specification.MipLevels, specification.ArrayLayers);

        return std::make_shared<VulkanTexture>(m_Device.GetDevice(), m_Allocator.GetAllocator(), specification, &m_UploadQueue);
    }

    std::shared_ptr<Texture> VulkanRendererAPI::LoadTextureFromFile(const std::string& path)
    {
        int l_Width = 0;
        int l_Height = 0;
        int l_Channels = 0;
        stbi_uc* l_Pixels = stbi_load(path.c_str(), &l_Width, &l_Height, &l_Channels, STBI_rgb_alpha);

        if (!l_Pixels)
        {
            TR_CORE_ERROR("LoadTextureFromFile: stbi_load failed for '{}'", path);
            return nullptr;
        }

        TextureSpecification l_Spec{};
        l_Spec.Width = static_cast<uint32_t>(l_Width);
        l_Spec.Height = static_cast<uint32_t>(l_Height);
        l_Spec.Format = TextureFormat::RGBA8;
        l_Spec.Usage = TextureUsage::Sampled;
        l_Spec.DebugName = path;

        auto a_Texture = CreateTexture(l_Spec);
        if (a_Texture == nullptr)
        {
            stbi_image_free(l_Pixels);
            return nullptr;
        }

        const uint64_t l_DataSize = static_cast<uint64_t>(l_Width) * static_cast<uint64_t>(l_Height) * 4ull;
        a_Texture->Upload(l_Pixels, l_DataSize, 0, 0);
        m_UploadQueue.FlushSync();

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
            TR_CORE_ERROR("CreateTextureFromMemory: stbi_load_from_memory failed ({} bytes)", size);
            return nullptr;
        }

        TextureSpecification l_Spec{};
        l_Spec.Width = static_cast<uint32_t>(l_Width);
        l_Spec.Height = static_cast<uint32_t>(l_Height);
        l_Spec.Format = TextureFormat::RGBA8;
        l_Spec.Usage = TextureUsage::Sampled;

        auto a_Texture = CreateTexture(l_Spec);
        if (a_Texture == nullptr)
        {
            stbi_image_free(l_Pixels);
            return nullptr;
        }

        const uint64_t l_DataSize = static_cast<uint64_t>(l_Width) * static_cast<uint64_t>(l_Height) * 4ull;
        a_Texture->Upload(l_Pixels, l_DataSize, 0, 0);
        m_UploadQueue.FlushSync();

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
        SamplerCacheKey l_Key{};
        l_Key.MinFilter = specification.MinFilter;
        l_Key.MagFilter = specification.MagFilter;
        l_Key.MipmapMode = specification.MipmapMode;
        l_Key.AddressModeU = specification.AddressModeU;
        l_Key.AddressModeV = specification.AddressModeV;
        l_Key.AddressModeW = specification.AddressModeW;
        l_Key.MipLodBias = specification.MipLodBias;
        l_Key.MinLod = specification.MinLod;
        l_Key.MaxLod = specification.MaxLod;
        l_Key.AnisotropyEnable = specification.AnisotropyEnable;
        l_Key.MaxAnisotropy = specification.MaxAnisotropy;
        l_Key.CompareEnable = specification.CompareEnable;
        l_Key.Compare = specification.Compare;
        l_Key.Border = specification.Border;
        l_Key.UnnormalizedCoordinates = specification.UnnormalizedCoordinates;

        auto a_Existing = m_SamplerCache.find(l_Key);
        if (a_Existing != m_SamplerCache.end())
        {
            return a_Existing->second;
        }

        auto l_Sampler = std::make_shared<VulkanSampler>(m_Device.GetDevice(), specification);
        m_SamplerCache.emplace(l_Key, l_Sampler);

        return l_Sampler;
    }

    std::shared_ptr<ComputePipeline> VulkanRendererAPI::CreateComputePipeline(const ComputePipelineSpecification& specification)
    {
        return std::make_shared<VulkanComputePipeline>(m_Device.GetDevice(), specification);
    }

    std::shared_ptr<DescriptorSetLayout> VulkanRendererAPI::CreateDescriptorSetLayout(const DescriptorSetLayoutSpecification& specification)
    {
        LayoutCacheKey l_Key;
        l_Key.Bindings = specification.Bindings;

        auto a_Existing = m_LayoutCache.find(l_Key);
        if (a_Existing != m_LayoutCache.end())
        {
            return a_Existing->second;
        }

        auto l_Layout = std::make_shared<VulkanDescriptorSetLayout>(m_Device.GetDevice(), specification);
        m_LayoutCache.emplace(std::move(l_Key), l_Layout);

        return l_Layout;
    }

    std::shared_ptr<QueryPool> VulkanRendererAPI::CreateQueryPool(const QueryPoolSpecification& specification)
    {
        return std::make_shared<VulkanQueryPool>(m_Device.GetDevice(), m_Device.GetProperties(), specification);
    }

    std::shared_ptr<DescriptorSet> VulkanRendererAPI::AllocateDescriptorSet(const std::shared_ptr<DescriptorSetLayout>& layout)
    {
        if (!layout)
        {
            TR_CORE_ERROR("AllocateDescriptorSet called with null layout");
            return nullptr;
        }

        auto* a_VulkanLayout = dynamic_cast<VulkanDescriptorSetLayout*>(layout.get());
        if (a_VulkanLayout == nullptr)
        {
            TR_CORE_ERROR("AllocateDescriptorSet: layout is not a VulkanDescriptorSetLayout");
            return nullptr;
        }

        VkDescriptorSet l_Set = m_DescriptorAllocator.AllocatePersistent(a_VulkanLayout->GetVkLayout());
        if (l_Set == VK_NULL_HANDLE)
        {
            return nullptr;
        }

        return std::make_shared<VulkanDescriptorSet>(m_Device.GetDevice(), l_Set, layout, &m_DescriptorAllocator, true);
    }

    std::shared_ptr<DescriptorSet> VulkanRendererAPI::AllocateTransientDescriptorSet(const std::shared_ptr<DescriptorSetLayout>& layout)
    {
        if (!layout)
        {
            TR_CORE_ERROR("AllocateTransientDescriptorSet called with null layout");
            return nullptr;
        }

        auto* a_VulkanLayout = dynamic_cast<VulkanDescriptorSetLayout*>(layout.get());
        if (a_VulkanLayout == nullptr)
        {
            TR_CORE_ERROR("AllocateTransientDescriptorSet: layout is not a VulkanDescriptorSetLayout");
            return nullptr;
        }

        VkDescriptorSet l_Set = m_DescriptorAllocator.AllocateTransient(a_VulkanLayout->GetVkLayout(), m_CurrentFrameIndex);
        if (l_Set == VK_NULL_HANDLE)
        {
            return nullptr;
        }

        return std::make_shared<VulkanDescriptorSet>(m_Device.GetDevice(), l_Set, layout, &m_DescriptorAllocator, false);
    }

    std::unique_ptr<RenderGraph> VulkanRendererAPI::CreateRenderGraph()
    {
        return std::make_unique<VulkanRenderGraph>(*this);
    }

    void VulkanRendererAPI::FlushUploads()
    {
        m_UploadQueue.FlushSync();
    }
}