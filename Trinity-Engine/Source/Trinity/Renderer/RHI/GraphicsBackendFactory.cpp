#include <Trinity/Renderer/RHI/GraphicsBackendFactory.h>

#include <Trinity/Core/Log.h>

#if defined(TRINITY_ENABLE_VULKAN)
#include <Trinity/Renderer/Backends/Vulkan/VulkanDevice.h>
#endif

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

    std::unique_ptr<GraphicsDevice> GraphicsBackendFactory::Create(const GraphicsDeviceDescription& description)
    {
        return Create(DetectBackend(), description);
    }

    std::unique_ptr<GraphicsDevice> GraphicsBackendFactory::Create(GraphicsBackend backend, const GraphicsDeviceDescription& description)
    {
        switch (backend)
        {
            case GraphicsBackend::Vulkan:
            {
#if defined(TRINITY_ENABLE_VULKAN)
                auto l_Device = std::make_unique<VulkanDevice>(description.Window, description.ApplicationName, description.EnableValidation);
                if (!l_Device->Initialize())
                {
                    TR_CORE_CRITICAL("GraphicsBackendFactory: Vulkan device initialization failed");
                    return nullptr;
                }

                return l_Device;
#else
                TR_CORE_ERROR("GraphicsBackendFactory: Vulkan backend not compiled in");
                return nullptr;
#endif
            }

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