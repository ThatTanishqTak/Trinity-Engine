#pragma once

#include <memory>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Renderer/RHI/GraphicsDevice.h>

namespace Trinity
{
    class GraphicsBackendFactory
    {
    public:
        static std::unique_ptr<GraphicsDevice> Create();
        static std::unique_ptr<GraphicsDevice> Create(GraphicsBackend backend);
    };
}