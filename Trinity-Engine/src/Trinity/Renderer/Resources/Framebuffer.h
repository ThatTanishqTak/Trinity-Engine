#pragma once

#include "Trinity/Renderer/CommandList.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Trinity
{
    struct FramebufferAttachmentSpecification
    {
        TextureFormat Format = TextureFormat::None;
        TextureUsage Usage = TextureUsage::None;
        uint32_t MipLevels = 1;
        uint32_t ArrayLayers = 1;
        uint32_t Samples = 1;
        std::string DebugName;
    };

    struct FramebufferSpecification
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        std::vector<FramebufferAttachmentSpecification> ColorAttachments;
        FramebufferAttachmentSpecification DepthAttachment;
        bool HasDepthAttachment = false;

        std::string DebugName;
    };

    class Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual std::shared_ptr<Texture> GetColorAttachment(uint32_t index = 0) const = 0;
        virtual std::shared_ptr<Texture> GetDepthAttachment() const = 0;
        virtual uint32_t GetColorAttachmentCount() const = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        const FramebufferSpecification& GetSpecification() const { return m_Specification; }

        RenderingInfo BuildRenderingInfo() const;

    protected:
        FramebufferSpecification m_Specification;
    };
}