#include "Trinity/Renderer/RenderGraph/RenderGraphPass.h"

#include <utility>

namespace Trinity
{
    RenderGraphPass::RenderGraphPass(std::string name) : m_Name(std::move(name))
    {

    }

    RenderGraphPass& RenderGraphPass::SetType(RenderGraphPassType type)
    {
        m_Type = type;

        return *this;
    }

    RenderGraphPass& RenderGraphPass::SetEnabled(bool enabled)
    {
        m_Enabled = enabled;

        return *this;
    }

    RenderGraphPass& RenderGraphPass::SetCullable(bool cullable)
    {
        m_Cullable = cullable;

        return *this;
    }

    RenderGraphPass& RenderGraphPass::SetDebugColor(float r, float g, float b, float a)
    {
        m_DebugColor = { r, g, b, a };

        return *this;
    }

    RenderGraphPass& RenderGraphPass::Read(RenderGraphResourceHandle resource, RenderGraphAccess access)
    {
        if (resource.IsValid() && access != RenderGraphAccess::None && !ContainsUsage(m_Reads, resource, access))
        {
            m_Reads.push_back({ resource, access });
        }

        return *this;
    }

    RenderGraphPass& RenderGraphPass::Write(RenderGraphResourceHandle resource, RenderGraphAccess access)
    {
        if (resource.IsValid() && access != RenderGraphAccess::None && !ContainsUsage(m_Writes, resource, access))
        {
            m_Writes.push_back({ resource, access });
        }

        return *this;
    }

    RenderGraphPass& RenderGraphPass::ReadWrite(RenderGraphResourceHandle resource, RenderGraphAccess access)
    {
        if (resource.IsValid() && access != RenderGraphAccess::None && !ContainsUsage(m_ReadWrites, resource, access))
        {
            m_ReadWrites.push_back({ resource, access });
        }

        return *this;
    }

    RenderGraphPass& RenderGraphPass::Read(RenderGraphResourceHandle resource)
    {
        if (!resource.IsValid())
        {
            return *this;
        }

        if (resource.IsTexture())
        {
            return Read(resource, RenderGraphAccess::ShaderSampledRead);
        }

        return Read(resource, RenderGraphAccess::StorageBufferRead);
    }

    RenderGraphPass& RenderGraphPass::Write(RenderGraphResourceHandle resource)
    {
        if (!resource.IsValid())
        {
            return *this;
        }

        if (resource.IsTexture())
        {
            return Write(resource, RenderGraphAccess::ColorAttachmentWrite);
        }

        return Write(resource, RenderGraphAccess::StorageBufferWrite);
    }

    RenderGraphPass& RenderGraphPass::SetExecuteCallback(std::function<void(RenderGraphContext&)> callback)
    {
        m_ExecuteCallback = std::move(callback);

        return *this;
    }

    void RenderGraphPass::Execute(RenderGraphContext& context) const
    {
        if (!m_Enabled)
        {
            return;
        }

        if (m_ExecuteCallback)
        {
            m_ExecuteCallback(context);
        }
    }

    bool RenderGraphPass::ContainsUsage(const std::vector<RenderGraphResourceUsage>& usages, RenderGraphResourceHandle resource, RenderGraphAccess access)
    {
        for (const auto& it_Usage : usages)
        {
            if (it_Usage.Resource == resource && it_Usage.Access == access)
            {
                return true;
            }
        }

        return false;
    }
}