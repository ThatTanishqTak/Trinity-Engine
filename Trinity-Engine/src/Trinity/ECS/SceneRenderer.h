#pragma once

#include "Trinity/ECS/Scene.h"

#include <cstdint>
#include <glm/mat4x4.hpp>

namespace Trinity
{
    class SceneRenderer
    {
    public:
        struct RenderStats
        {
            uint32_t DrawCalls = 0;
            uint32_t EntityCount = 0;
        };

        static void SetViewportSize(float width, float height);

        static void Render(Scene& scene, const glm::mat4& view, const glm::mat4& projection);
        static void Render(Scene& scene);

        static const RenderStats& GetStats();
        static void ResetStats();

    private:
        static void SubmitMeshes(Scene& scene, const glm::mat4& view, const glm::mat4& projection);

        static RenderStats s_Stats;
        static float s_ViewportWidth;
        static float s_ViewportHeight;
    };
}