#include "Trinity/Renderer/Resources/Framebuffer.h"

#include <utility>

namespace Trinity
{
    RenderingInfo Framebuffer::BuildRenderingInfo() const
    {
        RenderingInfo l_Info{};
        l_Info.Width = GetWidth();
        l_Info.Height = GetHeight();

        const uint32_t l_ColorCount = GetColorAttachmentCount();
        l_Info.ColorAttachments.reserve(l_ColorCount);

        for (uint32_t l_Index = 0; l_Index < l_ColorCount; ++l_Index)
        {
            RenderingAttachment l_Attachment{};
            l_Attachment.ColorTexture = GetColorAttachment(l_Index);
            l_Info.ColorAttachments.push_back(std::move(l_Attachment));
        }

        if (m_Specification.HasDepthAttachment)
        {
            l_Info.Depth.DepthTexture = GetDepthAttachment();
            l_Info.Depth.ClearOnLoad = true;
            l_Info.Depth.ClearDepth = 1.0f;
            l_Info.Depth.ClearStencil = 0;
        }

        return l_Info;
    }
}