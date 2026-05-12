#pragma once

#include "Trinity/Renderer/RenderGraph/RenderGraphResource.h"
#include "Trinity/Renderer/SceneRenderer.h"

#include <cstdint>
#include <vector>

namespace Trinity
{
    struct SceneRenderGraphResources
    {
        RenderGraphResourceHandle Albedo;
        RenderGraphResourceHandle Normal;
        RenderGraphResourceHandle MetallicRoughnessAO;
        RenderGraphResourceHandle Depth;
        RenderGraphResourceHandle ShadowMap;
        RenderGraphResourceHandle LitOutput;
    };

    struct SceneRenderPassContext
    {
        uint32_t Width = 0;
        uint32_t Height = 0;

        SceneRendererStats* Stats = nullptr;

        Camera* ActiveCamera = nullptr;
        SceneRenderData* SceneData = nullptr;

        std::vector<MeshDrawCommand>* DrawList = nullptr;
    };
}