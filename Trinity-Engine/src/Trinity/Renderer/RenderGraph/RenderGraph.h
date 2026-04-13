#pragma once

#include "Trinity/Renderer/RenderGraph/RenderGraphContext.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphPass.h"
#include "Trinity/Renderer/RenderGraph/RenderGraphResource.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Trinity
{
    class RenderGraph
    {
    public:
        RenderGraph() = default;
        virtual ~RenderGraph() = default;

        RenderGraph(const RenderGraph&) = delete;
        RenderGraph& operator=(const RenderGraph&) = delete;

        RenderGraphResourceHandle DeclareTexture(const RenderGraphTextureDescription& description);

        RenderGraphPass& AddPass(const std::string& name);

        void Reset();

        void Compile();
        void Execute();

        void OnResize(uint32_t width, uint32_t height);

        std::shared_ptr<Texture> GetTexture(RenderGraphResourceHandle handle) const;

        bool IsCompiled() const { return m_Compiled; }
        const std::vector<RenderGraphPass>& GetPasses() const { return m_Passes; }
        const std::vector<uint32_t>& GetExecutionOrder() const { return m_ExecutionOrder; }
        const std::vector<RenderGraphTextureDescription>& GetResourceDescs() const { return m_ResourceDescs; }

    protected:
        virtual std::shared_ptr<Texture> CreateResource(const RenderGraphTextureDescription& description) = 0;

        virtual void OnReset()
        {

        }

        virtual void OnCompile()
        {

        }

        virtual void OnExecutePassBegin(uint32_t passIndex, RenderGraphContext& context)
        {
            (void)passIndex;
            (void)context;
        }

        virtual void OnExecutePassEnd(uint32_t passIndex, RenderGraphContext& context)
        {
            (void)passIndex;
            (void)context;
        }

    protected:
        uint32_t m_SwapchainWidth = 0;
        uint32_t m_SwapchainHeight = 0;

        bool m_Compiled = false;

        std::vector<RenderGraphTextureDescription> m_ResourceDescs;
        std::vector<std::shared_ptr<Texture>> m_Resources;
        std::vector<RenderGraphPass> m_Passes;
        std::vector<uint32_t> m_ExecutionOrder;

    private:
        void BuildExecutionOrder();
        void AllocateResources();
    };
}