#pragma once

#include "Trinity/Renderer/Passes/SceneRenderPassContext.h"
#include "Trinity/Renderer/Resources/Pipeline.h"

#include <memory>

namespace Trinity
{
    class RenderGraph;
    struct RenderGraphContext;

    class GeometryPass
    {
    public:
        GeometryPass() = default;
        ~GeometryPass() = default;

        GeometryPass(const GeometryPass&) = delete;
        GeometryPass& operator=(const GeometryPass&) = delete;

        void Initialize();
        void Shutdown();

        void DeclareResources(RenderGraph& graph, SceneRenderGraphResources& resources);
        void AddToGraph(RenderGraph& graph, SceneRenderGraphResources& resources, SceneRenderPassContext& context);

        bool IsReady() const { return m_Pipeline != nullptr; }

    private:
        void Execute(RenderGraphContext& context, SceneRenderGraphResources& resources, SceneRenderPassContext& passContext);

    private:
        std::shared_ptr<Pipeline> m_Pipeline;
    };
}