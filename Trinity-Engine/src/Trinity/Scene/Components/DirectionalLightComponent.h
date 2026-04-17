#pragma once

#include <glm/glm.hpp>

namespace Trinity
{
    struct DirectionalLightComponent
    {
        glm::vec3 Color     = { 1.0f, 1.0f, 1.0f };
        float     Intensity = 1.0f;
    };
}
