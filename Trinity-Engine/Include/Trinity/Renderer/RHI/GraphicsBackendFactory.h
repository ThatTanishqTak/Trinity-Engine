#pragma once

#include <memory>
#include <string>

#include <Trinity/Platform/PlatformTypes.h>
#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Renderer/RHI/GraphicsDevice.h>

namespace Trinity
{
    struct GraphicsDeviceDescription
    {
        NativeWindowHandle Window;

        std::string ApplicationName;
        bool EnableValidation = false;
    };

    class GraphicsBackendFactory
    {
    public:
        static std::unique_ptr<GraphicsDevice> Create(const GraphicsDeviceDescription& description);
        static std::unique_ptr<GraphicsDevice> Create(GraphicsBackend backend, const GraphicsDeviceDescription& description);
    };
}