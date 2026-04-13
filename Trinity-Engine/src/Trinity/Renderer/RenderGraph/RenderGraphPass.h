#pragma once

#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"

#include "Trinity/Renderer/RenderGraph/RenderGraphResource.h"

#include <functional>
#include <string>
#include <vector>

namespace Trinity
{
    class RenderGraph;

    class RenderGraphPass
    {
    public:
        RenderGraphPass() = default;
        explicit RenderGraphPass(std::string name);
        
        RenderGraphPass& Read(RenderGraphResourceHandle resource);
        RenderGraphPass& Write(RenderGraphResourceHandle resource);
        RenderGraphPass& SetExecuteCallback(std::function<void(RenderGraphContext&)> callback);
        
        const std::string& GetName() const { return m_Name; }
        const std::vector<RenderGraphResourceHandle>& GetReads() const { return m_Reads; }
        const std::vector<RenderGraphResourceHandle>& GetWrites() const { return m_Writes; }
        void Execute(RenderGraphContext& context) const;

    private:
        std::string m_Name;
        std::vector<RenderGraphResourceHandle> m_Reads;
        std::vector<RenderGraphResourceHandle> m_Writes;
        std::function<void(RenderGraphContext&)> m_ExecuteCallback;
    };
}