#pragma once

#include "Trinity/Renderer/Camera/Camera.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <cstring>
#include <memory>

namespace Trinity
{
    namespace RendererUtilities
    {
        glm::mat4 Perspective(float fovDegrees, float aspectRatio, float nearClip, float farClip);
        glm::mat4 Orthographic(float left, float right, float bottom, float top, float nearClip, float farClip);

        void ComputeFrustumCornersWorld(const Camera& camera, glm::vec3 outCorners[8]);
        void ComputeFrustumCornersWorld(const glm::mat4& inverseViewProjection, glm::vec3 outCorners[8]);

        glm::vec3 RotateDirection(const glm::vec3& rotationRadians, const glm::vec3& baseDirection);

        inline void PackMat4(float dst[16], const glm::mat4& src) { std::memcpy(dst, &src[0][0], sizeof(float) * 16); }
        inline void PackVec4(float dst[4], const glm::vec4& src) { std::memcpy(dst, &src.x, sizeof(float) * 4); }
        inline void PackVec3(float dst[3], const glm::vec3& src) { std::memcpy(dst, &src.x, sizeof(float) * 3); }

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
    }
}