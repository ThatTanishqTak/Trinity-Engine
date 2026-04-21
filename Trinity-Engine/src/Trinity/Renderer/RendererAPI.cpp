#include "Trinity/Renderer/RendererAPI.h"

#include "Trinity/Utilities/Log.h"

#ifdef TRINITY_RENDERER_VULKAN
#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#endif

#include <cstdlib>

namespace Trinity
{
    std::unique_ptr<RendererAPI> RendererAPI::Create(RendererBackend backend)
    {
        switch (backend)
        {
#ifdef TRINITY_RENDERER_VULKAN
            case RendererBackend::Vulkan:
            {
                return std::make_unique<VulkanRendererAPI>();
            }
#endif
            default:
                TR_CORE_CRITICAL("Requested renderer backend is not compiled into this build");
                std::abort();

                return nullptr;
        }
    }
}