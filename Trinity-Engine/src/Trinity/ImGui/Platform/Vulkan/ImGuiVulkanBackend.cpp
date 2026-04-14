#include "Trinity/ImGui/Platform/Vulkan/ImGuiVulkanBackend.h"

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <SDL3/SDL.h>

namespace Trinity
{
    void ImGuiLayer::Implementation::Initialize(SDL_Window* window, VulkanRendererAPI& api)
    {
        Device = api.GetDevice().GetDevice();

        CreateDescriptorPool();
        CreateDefaultSampler();

        VkFormat l_SwapchainFormat = api.GetSwapchain().GetImageFormat();

        VkPipelineRenderingCreateInfoKHR l_PipelineRenderingInfo{};
        l_PipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        l_PipelineRenderingInfo.colorAttachmentCount = 1;
        l_PipelineRenderingInfo.pColorAttachmentFormats = &l_SwapchainFormat;

        ImGui_ImplVulkan_InitInfo l_VulkanInfo{};
        l_VulkanInfo.ApiVersion = VK_API_VERSION_1_3;
        l_VulkanInfo.Instance = api.GetDevice().GetInstance();
        l_VulkanInfo.PhysicalDevice = api.GetDevice().GetPhysicalDevice();
        l_VulkanInfo.Device = Device;
        l_VulkanInfo.QueueFamily = api.GetDevice().GetQueueFamilyIndices().GraphicsFamily.value();
        l_VulkanInfo.Queue = api.GetDevice().GetGraphicsQueue();
        l_VulkanInfo.DescriptorPool = DescriptorPool;
        l_VulkanInfo.MinImageCount = 2;
        l_VulkanInfo.ImageCount = api.GetSwapchain().GetImageCount();
        l_VulkanInfo.UseDynamicRendering = true;
        l_VulkanInfo.PipelineInfoMain.PipelineRenderingCreateInfo = l_PipelineRenderingInfo;
        l_VulkanInfo.PipelineInfoForViewports.PipelineRenderingCreateInfo = l_PipelineRenderingInfo;

        ImGui_ImplSDL3_InitForVulkan(window);
        ImGui_ImplVulkan_Init(&l_VulkanInfo);

        TR_CORE_INFO("ImGui Vulkan backend initialized");
    }

    void ImGuiLayer::Implementation::Shutdown()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();

        if (DefaultSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(Device, DefaultSampler, nullptr);
            DefaultSampler = VK_NULL_HANDLE;
        }

        if (DescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(Device, DescriptorPool, nullptr);
            DescriptorPool = VK_NULL_HANDLE;
        }
    }

    void ImGuiLayer::Implementation::NewFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
    }

    void ImGuiLayer::Implementation::Render()
    {
        auto& a_API = static_cast<VulkanRendererAPI&>(Renderer::GetAPI());

        VkCommandBuffer l_Cmd = a_API.GetCurrentCommandBuffer();
        uint32_t l_ImageIndex = a_API.GetCurrentImageIndex();
        VkImageView l_ImageView = a_API.GetSwapchain().GetImageView(l_ImageIndex);
        uint32_t l_Width = a_API.GetSwapchainWidth();
        uint32_t l_Height = a_API.GetSwapchainHeight();

        VkRenderingAttachmentInfo l_ColorAttachment{};
        l_ColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        l_ColorAttachment.imageView = l_ImageView;
        l_ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        l_ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        l_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        l_RenderingInfo.renderArea = { { 0, 0 }, { l_Width, l_Height } };
        l_RenderingInfo.layerCount = 1;
        l_RenderingInfo.colorAttachmentCount = 1;
        l_RenderingInfo.pColorAttachments = &l_ColorAttachment;

        vkCmdBeginRendering(l_Cmd, &l_RenderingInfo);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), l_Cmd);
        vkCmdEndRendering(l_Cmd);
    }

    void ImGuiLayer::Implementation::ProcessPlatformEvent(const void* sdlEvent)
    {
        ImGui_ImplSDL3_ProcessEvent(static_cast<const SDL_Event*>(sdlEvent));
    }

    uint64_t ImGuiLayer::Implementation::RegisterTexture(uint64_t opaqueImageViewHandle)
    {
        VkImageView l_View = reinterpret_cast<VkImageView>(opaqueImageViewHandle);
        VkDescriptorSet l_Set = ImGui_ImplVulkan_AddTexture(DefaultSampler, l_View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        return reinterpret_cast<uint64_t>(l_Set);
    }

    void ImGuiLayer::Implementation::UnregisterTexture(uint64_t textureID)
    {
        auto a_Set = reinterpret_cast<VkDescriptorSet>(reinterpret_cast<void*>(textureID));
        ImGui_ImplVulkan_RemoveTexture(a_Set);
    }

    void ImGuiLayer::Implementation::CreateDescriptorPool()
    {
        VkDescriptorPoolSize l_PoolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
        };

        VkDescriptorPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        l_PoolInfo.maxSets = 1000 * 11;
        l_PoolInfo.poolSizeCount = static_cast<uint32_t>(std::size(l_PoolSizes));
        l_PoolInfo.pPoolSizes = l_PoolSizes;

        VulkanUtilities::VKCheck(vkCreateDescriptorPool(Device, &l_PoolInfo, nullptr, &DescriptorPool), "Failed vkCreateDescriptorPool (ImGui)");
    }

    void ImGuiLayer::Implementation::CreateDefaultSampler()
    {
        VkSamplerCreateInfo l_SamplerInfo{};
        l_SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        l_SamplerInfo.magFilter = VK_FILTER_LINEAR;
        l_SamplerInfo.minFilter = VK_FILTER_LINEAR;
        l_SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        l_SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerInfo.maxLod = VK_LOD_CLAMP_NONE;

        VulkanUtilities::VKCheck(vkCreateSampler(Device, &l_SamplerInfo, nullptr, &DefaultSampler), "Failed vkCreateSampler (ImGui default)");
    }
}