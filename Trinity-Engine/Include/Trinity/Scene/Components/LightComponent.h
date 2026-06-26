#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

namespace Trinity
{
    enum class LightType : uint32_t
    {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    inline const char* LightTypeToString(LightType type)
    {
        switch (type)
        {
            case LightType::Point:
                return "Point";
            case LightType::Spot:
                return "Spot";
            case LightType::Directional:
            default:
                return "Directional";
        }
    }

    inline LightType LightTypeFromString(const std::string& text)
    {
        if (text == "Point")
        {
            return LightType::Point;
        }

        if (text == "Spot")
        {
            return LightType::Spot;
        }

        return LightType::Directional;
    }

    struct LightComponent
    {
        LightType Type = LightType::Directional;

        glm::vec3 Color{ 1.0f };
        float Intensity = 1.0f;

        // Point and spot lights only. Distance at which the light fully attenuates.
        float Range = 10.0f;

        // Spot lights only. Cone half-angles in radians (inner is fully lit, outer is the edge).
        float InnerConeAngle = 0.523599f;  // 30 degrees
        float OuterConeAngle = 0.698132f;  // 40 degrees
    };
}