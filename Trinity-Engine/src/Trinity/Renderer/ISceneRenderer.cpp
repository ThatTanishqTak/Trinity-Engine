#include "Trinity/Renderer/ISceneRenderer.h"

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Utilities/Log.h"

namespace Trinity
{
    std::unique_ptr<ISceneRenderer> CreateSceneRenderer()
    {
        RendererBackend l_SceneRenderer = Renderer::GetAPI().GetBackend();
        switch (l_SceneRenderer)
        {
            case RendererBackend::Vulkan:
                return CreateVulkanSceneRenderer();
            default:
                TR_CORE_ERROR("SceneRenderer  not supported");
                return nullptr;
        }
    }
}