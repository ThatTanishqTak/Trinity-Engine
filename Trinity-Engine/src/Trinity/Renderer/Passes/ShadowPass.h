#pragma once

#include "Trinity/Renderer/Passes/SceneRenderPassContext.h"
#include "Trinity/Renderer/Passes/ShadowTypes.h"
#include "Trinity/Renderer/RendererUtilities.h"
#include "Trinity/Renderer/Resources/Pipeline.h"

#include <array>
#include <memory>

namespace Trinity
{
    class RenderGraph;
    struct RenderGraphContext;

    class ShadowPass
    {
    public:
        void Initialize();
        void Shutdown();

        void DeclareResources(RenderGraph& graph, SceneRenderGraphResources& resources);
        void AddToGraph(RenderGraph& graph, SceneRenderGraphResources& resources, SceneRenderPassContext& context);

        bool IsReady() const { return m_OpaquePipeline != nullptr; }

        void SetSettings(const ShadowSettings& settings);
        const ShadowSettings& GetSettings() const { return m_Settings; }

    private:
        void Execute(RenderGraphContext& context, SceneRenderGraphResources& resources, SceneRenderPassContext& passContext);

    private:
        ShadowSettings m_Settings;
        std::shared_ptr<Pipeline> m_OpaquePipeline;
        std::array<RendererUtilities::CascadeData, 4> m_Cascades;
    };
}