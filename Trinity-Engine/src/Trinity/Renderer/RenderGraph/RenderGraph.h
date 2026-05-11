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
    struct ResourceLifetime
    {
        uint32_t FirstUse = UINT32_MAX;
        uint32_t LastUse = 0;

        bool IsValid() const
        {
            return FirstUse <= LastUse && FirstUse != UINT32_MAX;
        }
    };

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
        const std::vector<RenderGraphTextureDescription>& GetResourceDescription() const { return m_ResourceDescription; }
        const std::vector<ResourceLifetime>& GetLifetimes() const { return m_Lifetimes; }

    protected:
        virtual std::vector<std::shared_ptr<Texture>> AllocateResourceBatch(const std::vector<RenderGraphTextureDescription>& descriptions, const std::vector<ResourceLifetime>& lifetimes) = 0;

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

        std::vector<RenderGraphTextureDescription> m_ResourceDescription;
        std::vector<std::shared_ptr<Texture>> m_Resources;
        std::vector<RenderGraphPass> m_Passes;
        std::vector<uint32_t> m_ExecutionOrder;
        std::vector<ResourceLifetime> m_Lifetimes;

    private:
        void BuildExecutionOrder();
        void ComputeLifetimes();
        void AllocateResources();
    };
}