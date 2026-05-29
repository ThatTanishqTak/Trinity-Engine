#include <Trinity/Renderer/RHI/GraphicsBackendFactory.h>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    static GraphicsBackend DetectBackend()
    {
#if defined(TRINITY_ENABLE_VULKAN)
        return GraphicsBackend::Vulkan;
#elif defined(TRINITY_ENABLE_METAL)
        return GraphicsBackend::Metal;
#elif defined(TRINITY_ENABLE_DIRECTX12)
        return GraphicsBackend::DirectX12;
#else
        return GraphicsBackend::None;
#endif
    }

    std::unique_ptr<GraphicsDevice> GraphicsBackendFactory::Create()
    {
        return Create(DetectBackend());
    }

    std::unique_ptr<GraphicsDevice> GraphicsBackendFactory::Create(GraphicsBackend backend)
    {
        switch (backend)
        {
            case GraphicsBackend::Vulkan:
                TR_CORE_ERROR("GraphicsBackendFactory: Vulkan backend arrives in Milestone 7");
                return nullptr;

            case GraphicsBackend::Metal:
                TR_CORE_ERROR("GraphicsBackendFactory: Metal backend arrives in Milestone 28");
                return nullptr;

            case GraphicsBackend::DirectX12:
                TR_CORE_ERROR("GraphicsBackendFactory: DirectX12 backend not yet implemented");
                return nullptr;

            default:
                TR_CORE_ERROR("GraphicsBackendFactory: no graphics backend available");
                return nullptr;
        }
    }
}