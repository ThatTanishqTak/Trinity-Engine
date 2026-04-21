#pragma once

#include <glm/glm.hpp>

namespace Trinity
{
    struct DirectionalLightComponent
    {
        glm::vec3 Direction = { 0.0f, -1.0f, 0.0f };
        glm::vec3 Color = { 1.0f, 1.0f, 1.0f };

        float Intensity = 1.0f;
    };

    struct PointLightComponent
    {
        glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Color = { 1.0f, 1.0f, 1.0f };

        float Intensity = 1.0f;
    };

    struct SpotLightComponent
    {

    };

    struct RectLightComponent
    {

    };

    struct CapsuleLightComponent
    {

    };

    struct SphereLightComponent
    {

    };
}