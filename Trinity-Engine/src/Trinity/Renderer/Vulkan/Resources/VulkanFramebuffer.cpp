#include "Trinity/Renderer/Vulkan/Resources/VulkanFramebuffer.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

#include <string>

namespace Trinity
{
    namespace
    {
        TextureUsage ResolveColorUsage(const FramebufferAttachmentSpecification& spec)
        {
            if (spec.Usage == TextureUsage::None)
            {
                return TextureUsage::ColorAttachment | TextureUsage::Sampled;
            }

            return spec.Usage | TextureUsage::ColorAttachment;
        }

        TextureUsage ResolveDepthUsage(const FramebufferAttachmentSpecification& spec)
        {
            if (spec.Usage == TextureUsage::None)
            {
                return TextureUsage::DepthStencilAttachment | TextureUsage::Sampled;
            }

            return spec.Usage | TextureUsage::DepthStencilAttachment;
        }

        std::string ResolveAttachmentDebugName(const FramebufferSpecification& fbSpec, const FramebufferAttachmentSpecification& spec, const std::string& suffix)
        {
            if (!spec.DebugName.empty())
            {
                return spec.DebugName;
            }

            if (!fbSpec.DebugName.empty())
            {
                return fbSpec.DebugName + "::" + suffix;
            }

            return std::string{};
        }

        const char* DescribeName(const std::string& name)
        {
            return name.empty() ? "<unnamed>" : name.c_str();
        }
    }

    VulkanFramebuffer::VulkanFramebuffer(VkDevice device, VmaAllocator allocator, const FramebufferSpecification& specification) : m_Device(device), m_Allocator(allocator)
    {
        TR_CORE_TRACE("Creating Vulkan Framebuffer '{}' ({}x{}, color={}, depth={})", DescribeName(specification.DebugName), specification.Width, specification.Height, specification.ColorAttachments.size(), specification.HasDepthAttachment ? "yes" : "no");

        m_Specification = specification;

        if (m_Specification.Width == 0 || m_Specification.Height == 0)
        {
            TR_CORE_WARN("Vulkan Framebuffer '{}' constructed with zero dimension ({}x{}); attachment creation deferred until Resize", DescribeName(m_Specification.DebugName), m_Specification.Width, m_Specification.Height);
            return;
        }

        CreateAttachments();

        TR_CORE_TRACE("Vulkan Framebuffer '{}' Created", DescribeName(m_Specification.DebugName));
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        TR_CORE_TRACE("Destroying Vulkan Framebuffer '{}'", DescribeName(m_Specification.DebugName));

        DestroyAttachments();
    }

    void VulkanFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            TR_CORE_WARN("Vulkan Framebuffer '{}' resize rejected: zero dimension ({}x{})", DescribeName(m_Specification.DebugName), width, height);
            return;
        }

        const bool l_DimensionsUnchanged = (m_Specification.Width == width) && (m_Specification.Height == height);
        const bool l_AlreadyPopulated = !m_ColorAttachments.empty() || m_DepthAttachment != nullptr;

        if (l_DimensionsUnchanged && l_AlreadyPopulated)
        {
            return;
        }

        TR_CORE_TRACE("Resizing Vulkan Framebuffer '{}': {}x{} -> {}x{}", DescribeName(m_Specification.DebugName), m_Specification.Width, m_Specification.Height, width, height);

        m_Specification.Width = width;
        m_Specification.Height = height;

        DestroyAttachments();
        CreateAttachments();

        TR_CORE_TRACE("Vulkan Framebuffer '{}' Resized", DescribeName(m_Specification.DebugName));
    }

    std::shared_ptr<Texture> VulkanFramebuffer::GetColorAttachment(uint32_t index) const
    {
        if (index < m_ColorAttachments.size())
        {
            return m_ColorAttachments[index];
        }

        TR_CORE_WARN("Vulkan Framebuffer '{}' GetColorAttachment({}) out of range (count={})", DescribeName(m_Specification.DebugName), index, m_ColorAttachments.size());
        return nullptr;
    }

    std::shared_ptr<Texture> VulkanFramebuffer::GetDepthAttachment() const
    {
        return m_DepthAttachment;
    }

    void VulkanFramebuffer::CreateAttachments()
    {
        m_ColorAttachments.reserve(m_Specification.ColorAttachments.size());

        for (size_t l_Index = 0; l_Index < m_Specification.ColorAttachments.size(); ++l_Index)
        {
            const FramebufferAttachmentSpecification& l_AttachSpec = m_Specification.ColorAttachments[l_Index];

            TextureSpecification l_TextureSpec{};
            l_TextureSpec.Width = m_Specification.Width;
            l_TextureSpec.Height = m_Specification.Height;
            l_TextureSpec.MipLevels = l_AttachSpec.MipLevels > 0 ? l_AttachSpec.MipLevels : 1;
            l_TextureSpec.ArrayLayers = l_AttachSpec.ArrayLayers > 0 ? l_AttachSpec.ArrayLayers : 1;
            l_TextureSpec.Samples = l_AttachSpec.Samples > 0 ? l_AttachSpec.Samples : 1;
            l_TextureSpec.Format = l_AttachSpec.Format;
            l_TextureSpec.Usage = ResolveColorUsage(l_AttachSpec);
            l_TextureSpec.DebugName = ResolveAttachmentDebugName(m_Specification, l_AttachSpec, "Color" + std::to_string(l_Index));

            TR_CORE_DEBUG("Framebuffer '{}' color[{}] '{}': {}x{} mips={} layers={} samples={}", DescribeName(m_Specification.DebugName), l_Index, DescribeName(l_TextureSpec.DebugName), l_TextureSpec.Width, l_TextureSpec.Height, l_TextureSpec.MipLevels, l_TextureSpec.ArrayLayers, l_TextureSpec.Samples);

            m_ColorAttachments.push_back(std::make_shared<VulkanTexture>(m_Device, m_Allocator, l_TextureSpec));
        }

        if (m_Specification.HasDepthAttachment)
        {
            const FramebufferAttachmentSpecification& l_AttachSpec = m_Specification.DepthAttachment;

            TextureSpecification l_TextureSpec{};
            l_TextureSpec.Width = m_Specification.Width;
            l_TextureSpec.Height = m_Specification.Height;
            l_TextureSpec.MipLevels = l_AttachSpec.MipLevels > 0 ? l_AttachSpec.MipLevels : 1;
            l_TextureSpec.ArrayLayers = l_AttachSpec.ArrayLayers > 0 ? l_AttachSpec.ArrayLayers : 1;
            l_TextureSpec.Samples = l_AttachSpec.Samples > 0 ? l_AttachSpec.Samples : 1;
            l_TextureSpec.Format = l_AttachSpec.Format;
            l_TextureSpec.Usage = ResolveDepthUsage(l_AttachSpec);
            l_TextureSpec.DebugName = ResolveAttachmentDebugName(m_Specification, l_AttachSpec, "Depth");

            TR_CORE_DEBUG("Framebuffer '{}' depth '{}': {}x{} mips={} layers={} samples={}", DescribeName(m_Specification.DebugName), DescribeName(l_TextureSpec.DebugName), l_TextureSpec.Width, l_TextureSpec.Height, l_TextureSpec.MipLevels, l_TextureSpec.ArrayLayers, l_TextureSpec.Samples);

            m_DepthAttachment = std::make_shared<VulkanTexture>(m_Device, m_Allocator, l_TextureSpec);
        }
    }

    void VulkanFramebuffer::DestroyAttachments()
    {
        m_ColorAttachments.clear();
        m_DepthAttachment.reset();
    }
}