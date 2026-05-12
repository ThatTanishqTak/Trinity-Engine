#pragma once

#include "Trinity/Renderer/RenderGraph/RenderGraphResource.h"
#include "Trinity/Renderer/Resources/Buffer.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <limits>
#include <memory>

namespace Trinity
{
    class RenderGraph;
    class CommandList;

    struct RenderGraphContext
    {
        RenderGraph* Graph = nullptr;
        CommandList* Command = nullptr;

        uint32_t Width = 0;
        uint32_t Height = 0;

        uint32_t PassIndex = std::numeric_limits<uint32_t>::max();
        const char* PassName = nullptr;

        std::shared_ptr<Texture> GetTexture(RenderGraphResourceHandle handle) const;
        std::shared_ptr<Buffer> GetBuffer(RenderGraphResourceHandle handle) const;

        CommandList& GetCommandList() const;
    };
}