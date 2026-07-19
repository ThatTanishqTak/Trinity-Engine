#include <Trinity/Renderer/Backends/Vulkan/VulkanImGuiBackend.h>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <Trinity/Renderer/Backends/Vulkan/VulkanDevice.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanCommandList.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanUtilities.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    VulkanImGuiBackend::VulkanImGuiBackend(VulkanDevice& device) : m_Device(device)
    {

    }

    VulkanImGuiBackend::~VulkanImGuiBackend()
    {
        Shutdown();
    }

    bool VulkanImGuiBackend::Initialize(uint32_t framesInFlight, Format colorFormat)
    {
        if (m_Initialized)
        {
            return true;
        }

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
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        const uint32_t l_PoolTypeCount = static_cast<uint32_t>(sizeof(l_PoolSizes) / sizeof(l_PoolSizes[0]));

        VkDescriptorPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        l_PoolInfo.maxSets = 1000 * l_PoolTypeCount;
        l_PoolInfo.poolSizeCount = l_PoolTypeCount;
        l_PoolInfo.pPoolSizes = l_PoolSizes;

        if (vkCreateDescriptorPool(m_Device.GetHandle(), &l_PoolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
        {
            ("VulkanImGuiBackend: descriptor pool creation failed");

            return false;
        }

        m_ColorFormat = VulkanUtilities::ToVkFormat(colorFormat);

        uint32_t l_ImageCount = framesInFlight < 2 ? 2 : framesInFlight;

        ImGui_ImplVulkan_InitInfo l_InitInfo{};
        l_InitInfo.Instance = m_Device.GetInstance();
        l_InitInfo.PhysicalDevice = m_Device.GetPhysicalDevice();
        l_InitInfo.Device = m_Device.GetHandle();
        l_InitInfo.QueueFamily = m_Device.GetGraphicsQueueFamily();
        l_InitInfo.Queue = m_Device.GetGraphicsQueue();
        l_InitInfo.DescriptorPool = m_DescriptorPool;
        l_InitInfo.MinImageCount = l_ImageCount;
        l_InitInfo.ImageCount = l_ImageCount;
        l_InitInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        l_InitInfo.UseDynamicRendering = true;
        l_InitInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        l_InitInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        l_InitInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_ColorFormat;

        if (!ImGui_ImplVulkan_Init(&l_InitInfo))
        {
            ("VulkanImGuiBackend: ImGui_ImplVulkan_Init failed");
            vkDestroyDescriptorPool(m_Device.GetHandle(), m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;

            return false;
        }

        VkSamplerCreateInfo l_SamplerInfo{};
        l_SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        l_SamplerInfo.magFilter = VK_FILTER_LINEAR;
        l_SamplerInfo.minFilter = VK_FILTER_LINEAR;
        l_SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        l_SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        l_SamplerInfo.maxAnisotropy = 1.0f;
        l_SamplerInfo.minLod = -1000.0f;
        l_SamplerInfo.maxLod = 1000.0f;

        if (vkCreateSampler(m_Device.GetHandle(), &l_SamplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
        {
            ("VulkanImGuiBackend: sampler creation failed");
            ImGui_ImplVulkan_Shutdown();
            vkDestroyDescriptorPool(m_Device.GetHandle(), m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;

            return false;
        }

        m_Initialized = true;
        ("VulkanImGuiBackend: initialized");

        return true;
    }

    void VulkanImGuiBackend::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        vkDeviceWaitIdle(m_Device.GetHandle());
        ImGui_ImplVulkan_Shutdown();

        if (m_Sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_Device.GetHandle(), m_Sampler, nullptr);
            m_Sampler = VK_NULL_HANDLE;
        }

        if (m_DescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_Device.GetHandle(), m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }

        m_Initialized = false;
    }

    void VulkanImGuiBackend::NewFrame()
    {
        ImGui_ImplVulkan_NewFrame();
    }

    void VulkanImGuiBackend::RecordDrawData(CommandList& commandList)
    {
        ImDrawData* l_DrawData = ImGui::GetDrawData();
        if (l_DrawData == nullptr)
        {
            return;
        }

        VulkanCommandList& l_VulkanCommandList = static_cast<VulkanCommandList&>(commandList);
        ImGui_ImplVulkan_RenderDrawData(l_DrawData, l_VulkanCommandList.GetHandle());
    }

    uint64_t VulkanImGuiBackend::RegisterTexture(TextureHandle a_Texture)
    {
        if (!m_Initialized)
        {
            return 0;
        }

        VulkanTextureResource* l_Texture = m_Device.GetTexture(a_Texture);
        if (l_Texture == nullptr || l_Texture->View == VK_NULL_HANDLE)
        {
            return 0;
        }

        VkDescriptorSet l_Set = ImGui_ImplVulkan_AddTexture(m_Sampler, l_Texture->View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        return reinterpret_cast<uint64_t>(l_Set);
    }

    void VulkanImGuiBackend::UnregisterTexture(uint64_t a_TextureId)
    {
        if (!m_Initialized || a_TextureId == 0)
        {
            return;
        }

        ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(a_TextureId));
    }
}