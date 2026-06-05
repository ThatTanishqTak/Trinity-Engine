#pragma once

#include <vulkan/vulkan.h>

#include <Trinity/ImGui/IImGuiRenderBackend.h>

namespace Trinity
{
    class VulkanDevice;

    class VulkanImGuiBackend : public IImGuiRenderBackend
    {
    public:
        explicit VulkanImGuiBackend(VulkanDevice& device);
        ~VulkanImGuiBackend() override;

        VulkanImGuiBackend(const VulkanImGuiBackend&) = delete;
        VulkanImGuiBackend& operator=(const VulkanImGuiBackend&) = delete;

        bool Initialize(uint32_t framesInFlight, Format colorFormat) override;
        void Shutdown() override;

        void NewFrame() override;
        void RecordDrawData(CommandList& commandList) override;

        uint64_t RegisterTexture(TextureHandle texture) override;
        void UnregisterTexture(uint64_t textureID) override;

    private:
        VulkanDevice& m_Device;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;
        VkFormat m_ColorFormat = VK_FORMAT_UNDEFINED;
        bool m_Initialized = false;
    };
}