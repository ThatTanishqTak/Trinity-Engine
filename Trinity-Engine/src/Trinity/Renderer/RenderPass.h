#pragma once

namespace Trinity
{
    class RenderGraph;
    struct RenderGraphContext;

    class RenderPass
    {
    public:
        RenderPass() = default;
        virtual ~RenderPass() = default;

        RenderPass(const RenderPass&) = delete;
        RenderPass& operator=(const RenderPass&) = delete;

        virtual const char* GetName() const = 0;
        virtual void Setup(RenderGraph& graph) = 0;
        virtual void Execute(RenderGraphContext& context) = 0;
        virtual bool IsEnabled() const { return true; }
    };
}