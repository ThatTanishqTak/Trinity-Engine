#include "Trinity/Renderer/RenderGraph/RenderGraphPass.h"

#include <utility>

namespace Trinity
{
    RenderGraphPass::RenderGraphPass(std::string name) : m_Name(std::move(name))
    {

    }

    RenderGraphPass& RenderGraphPass::Read(RenderGraphResourceHandle resource)
    {
        if (resource.IsValid())
        {
            m_Reads.push_back(resource);
        }
    
        return *this;
    }

    RenderGraphPass& RenderGraphPass::Write(RenderGraphResourceHandle resource)
    {
        if (resource.IsValid())
        {
            m_Writes.push_back(resource);
        }
    
        return *this;
    }

    RenderGraphPass& RenderGraphPass::SetExecuteCallback(std::function<void(RenderGraphContext&)> callback)
    {
        m_ExecuteCallback = std::move(callback);

        return *this;
    }

    void RenderGraphPass::Execute(RenderGraphContext& context) const
    {
        if (m_ExecuteCallback)
        {
            m_ExecuteCallback(context);
        }
    }
}