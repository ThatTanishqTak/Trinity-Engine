#pragma once

#include "Trinity/ECS/Scene.h"
#include "Trinity/Renderer/Vulkan/VulkanShaderInterop.h"

#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace Trinity
{
    class SceneRenderer
    {
    public:
        struct RenderStats
        {
            uint32_t DrawCalls = 0;
            uint32_t EntityCount = 0;
            uint32_t LightCount = 0;
        };

        static void SetViewportSize(float width, float height);

        static void Render(Scene& scene, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPosition, float cameraNear, float cameraFar);
        static void Render(Scene& scene);

        static const RenderStats& GetStats();
        static void ResetStats();

    private:
        static void CollectLights(Scene& scene);
        static void SubmitShadowDraws(Scene& scene);
        static void SubmitGeometryDraws(Scene& scene, const glm::mat4& view, const glm::mat4& projection);
        static void SubmitLighting(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPosition, float cameraNear, float cameraFar);

    private:
        static RenderStats s_Stats;
        static float s_ViewportWidth;
        static float s_ViewportHeight;

        static std::vector<LightData> s_LightBuffer;
        static glm::mat4 s_DirectionalLightSpaceMatrix;
        static bool s_HasShadowCaster;
    };
}