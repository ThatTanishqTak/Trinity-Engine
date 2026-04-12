#include "Trinity/Renderer/Vulkan/Resources/VulkanFramebuffer.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

namespace Trinity
{
    VulkanFramebuffer::VulkanFramebuffer(VkDevice device, VmaAllocator allocator, const FramebufferSpecification& specification) : m_Device(device), m_Allocator(allocator)
    {
        m_Specification = specification;
        CreateAttachments();
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        DestroyAttachments();
    }

    void VulkanFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        if (m_Specification.Width == width && m_Specification.Height == height)
        {
            return;
        }

        m_Specification.Width = width;
        m_Specification.Height = height;

        DestroyAttachments();
        CreateAttachments();
    }

    std::shared_ptr<Texture> VulkanFramebuffer::GetColorAttachment(uint32_t index) const
    {
        if (index < m_ColorAttachments.size())
        {
            return m_ColorAttachments[index];
        }

        return nullptr;
    }

    std::shared_ptr<Texture> VulkanFramebuffer::GetDepthAttachment() const
    {
        return m_DepthAttachment;
    }

    void VulkanFramebuffer::CreateAttachments()
    {
        for (const auto& it_AttachSpecification : m_Specification.ColorAttachments)
        {
            TextureSpecification l_TexSpec{};
            l_TexSpec.Width = m_Specification.Width;
            l_TexSpec.Height = m_Specification.Height;
            l_TexSpec.Format = it_AttachSpecification.Format;
            l_TexSpec.Usage = TextureUsage::ColorAttachment | TextureUsage::Sampled;
            l_TexSpec.DebugName = it_AttachSpecification.DebugName;

            m_ColorAttachments.push_back(std::make_shared<VulkanTexture>(m_Device, m_Allocator, l_TexSpec));
        }

        if (m_Specification.HasDepthAttachment)
        {
            TextureSpecification l_DepthSpecification{};
            l_DepthSpecification.Width = m_Specification.Width;
            l_DepthSpecification.Height = m_Specification.Height;
            l_DepthSpecification.Format = m_Specification.DepthAttachment.Format;
            l_DepthSpecification.Usage = TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
            l_DepthSpecification.DebugName = m_Specification.DepthAttachment.DebugName;

            m_DepthAttachment = std::make_shared<VulkanTexture>(m_Device, m_Allocator, l_DepthSpecification);
        }
    }

    void VulkanFramebuffer::DestroyAttachments()
    {
        m_ColorAttachments.clear();
        m_DepthAttachment.reset();
    }
}