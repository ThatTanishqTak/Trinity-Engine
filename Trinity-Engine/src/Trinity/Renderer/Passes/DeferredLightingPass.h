#pragma once

#include "Trinity/Renderer/Passes/SceneRenderPassContext.h"
#include "Trinity/Renderer/Resources/DescriptorSet.h"
#include "Trinity/Renderer/Resources/DescriptorSetLayout.h"
#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/Sampler.h"

#include <memory>

namespace Trinity
{
    class RenderGraph;
    struct RenderGraphContext;

    class DeferredLightingPass
    {
    public:
        DeferredLightingPass() = default;
        ~DeferredLightingPass() = default;

        DeferredLightingPass(const DeferredLightingPass&) = delete;
        DeferredLightingPass& operator=(const DeferredLightingPass&) = delete;

        void Initialize();
        void Shutdown();

        void DeclareResources(RenderGraph& graph, SceneRenderGraphResources& resources);
        void AddToGraph(RenderGraph& graph, SceneRenderGraphResources& resources, SceneRenderPassContext& context);

        bool IsReady() const { return m_Pipeline != nullptr && m_DescriptorSetLayout != nullptr && m_Sampler != nullptr; }

    private:
        void Execute(RenderGraphContext& context, SceneRenderGraphResources& resources, SceneRenderPassContext& passContext);
        void WriteDescriptors(SceneRenderGraphResources& resources, RenderGraphContext& context);

    private:
        std::shared_ptr<Pipeline> m_Pipeline;
        std::shared_ptr<DescriptorSetLayout> m_DescriptorSetLayout;
        std::shared_ptr<DescriptorSet> m_DescriptorSet;
        std::shared_ptr<Sampler> m_Sampler;
    };
}