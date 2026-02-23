#pragma once

#include "Trinity/ECS/Scene.h"

#include <glm/mat4x4.hpp>

namespace Trinity
{
    class SceneRenderer
    {
    public:
        static void Render(Scene& scene, const glm::mat4& view, const glm::mat4& projection);
    };
}