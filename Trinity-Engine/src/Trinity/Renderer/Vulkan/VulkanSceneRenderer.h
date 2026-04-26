#pragma once

#include "Trinity/Renderer/SceneRenderer.h"

namespace Trinity
{
    class VulkanSceneRenderer : public SceneRenderer
    {
    public:
        void BeginScene() override;
        void EndScene() override;
    };
}