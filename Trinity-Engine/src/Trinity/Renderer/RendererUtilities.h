#pragma once

#include "Trinity/Renderer/Camera/Camera.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <glm/glm.hpp>

#include <entt/entt.hpp>

#include <cstdint>
#include <cstring>
#include <memory>

namespace Trinity
{
    namespace RendererUtilities
    {
        constexpr uint32_t ShadowMapResolution = 2048;
        constexpr float ShadowRadius = 30.0f;

        glm::mat4 Perspective(float fovDegrees, float aspectRatio, float nearClip, float farClip);
        glm::mat4 Orthographic(float left, float right, float bottom, float top, float nearClip, float farClip);

        void ComputeFrustumCornersWorld(const Camera& camera, glm::vec3 outCorners[8]);
        void ComputeFrustumCornersWorld(const glm::mat4& inverseViewProjection, glm::vec3 outCorners[8]);

        glm::mat4 ComputeDirectionalLightViewProjection(const Camera& camera, const glm::vec3& sunDirection, uint32_t shadowMapResolution, float shadowRadius);

        struct CascadeData
        {
            glm::mat4 ViewProjection = glm::mat4(1.0f);
            float SplitDepthView = 0.0f;
        };

        void ComputeCascadeSplits(float nearClip, float farClip, uint32_t cascadeCount, float lambda, float* outSplits);
        CascadeData ComputeCascadeMatrix(const Camera& camera, const glm::vec3& sunDirection, float splitNear, float splitFar, uint32_t cascadeResolution);

        glm::vec3 RotateDirection(const glm::vec3& rotationRadians, const glm::vec3& baseDirection);

        inline void PackMat4(float dst[16], const glm::mat4& src)
        {
            std::memcpy(dst, &src[0][0], sizeof(float) * 16);
        }

        inline void PackVec4(float dst[4], const glm::vec4& src)
        {
            std::memcpy(dst, &src.x, sizeof(float) * 4);
        }

        inline void PackVec3(float dst[3], const glm::vec3& src)
        {
            std::memcpy(dst, &src.x, sizeof(float) * 3);
        }

        inline void PackVec3AsVec4(float dst[4], const glm::vec3& src, float w)
        {
            dst[0] = src.x;
            dst[1] = src.y;
            dst[2] = src.z;
            dst[3] = w;
        }

        std::shared_ptr<Texture> CreateWhiteTexture();
        std::shared_ptr<Texture> CreateBlackTexture();
        std::shared_ptr<Texture> CreateDefaultNormalTexture();
        std::shared_ptr<Texture> CreateCheckerboardTexture(uint32_t width = 64, uint32_t height = 64);

        struct DirectionalSunData
        {
            glm::vec3 Direction = glm::vec3(0.0f, -1.0f, 0.0f);
            glm::vec3 Color = glm::vec3(1.0f);
            float Intensity = 1.0f;
            bool Valid = false;
        };

        DirectionalSunData CollectDirectionalSun(const entt::registry& registry);
    }
}