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

        bool IsReady() const { return m_Pipeline != nullptr && m_DescriptorSetLayout != nullptr && m_Sampler != nullptr; }

    private:
        void Execute(RenderGraphContext& context, SceneRenderGraphResources& resources, SceneRenderPassContext& passContext);
        void WriteMaterialDescriptors(RenderGraphContext& context, const MeshDrawCommand& drawCommand);

    private:
        std::shared_ptr<Pipeline> m_Pipeline;
        std::shared_ptr<DescriptorSetLayout> m_DescriptorSetLayout;
        std::shared_ptr<DescriptorSet> m_DescriptorSet;
        std::shared_ptr<Sampler> m_Sampler;

        std::shared_ptr<Texture> m_WhiteTexture;
        std::shared_ptr<Texture> m_DefaultNormalTexture;
    };
}