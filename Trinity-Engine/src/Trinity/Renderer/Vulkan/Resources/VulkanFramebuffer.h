#pragma once

#include "Trinity/Renderer/Resources/Framebuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

namespace Trinity
{
    class VulkanFramebuffer final : public Framebuffer
    {
    public:
        VulkanFramebuffer(VkDevice device, VmaAllocator allocator, const FramebufferSpecification& specification);
        ~VulkanFramebuffer() override;

        void Resize(uint32_t width, uint32_t height) override;

        std::shared_ptr<Texture> GetColorAttachment(uint32_t index = 0) const override;
        std::shared_ptr<Texture> GetDepthAttachment() const override;
        uint32_t GetColorAttachmentCount() const override { return static_cast<uint32_t>(m_ColorAttachments.size()); }

        uint32_t GetWidth() const override { return m_Specification.Width; }
        uint32_t GetHeight() const override { return m_Specification.Height; }

    private:
        void CreateAttachments();
        void DestroyAttachments();

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        std::vector<std::shared_ptr<VulkanTexture>> m_ColorAttachments;
        std::shared_ptr<VulkanTexture> m_DepthAttachment;
    };
}