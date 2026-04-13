#pragma once

#include "Trinity/Renderer/RenderGraph/RenderGraphResource.h"

#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <memory>

namespace Trinity
{
    class RenderGraph;

    struct RenderGraphContext
    {
        RenderGraph* Graph = nullptr;
        
        uint32_t Width = 0;
        uint32_t Height = 0;

        std::shared_ptr<Texture> GetTexture(RenderGraphResourceHandle handle) const;
    };
}