#pragma once

#include <cstdint>

namespace Trinity
{
    struct DirectionalShadowSettings
    {
        bool Enabled = true;
        uint32_t CascadeCount = 4;
        uint32_t AtlasResolution = 4096;
        float SplitLambda = 0.75f;
        float DepthBiasConstant = 1.25f;
        float DepthBiasSlope = 1.75f;
        float MaxShadowDistance = 200.0f;
        uint32_t PCFKernelRadius = 2;
    };

    struct SpotShadowSettings
    {
        bool Enabled = false;
        uint32_t MaxShadowedSpots = 16;
        uint32_t PerLightResolution = 1024;
    };

    struct PointShadowSettings
    {
        bool Enabled = false;
        uint32_t MaxShadowedPoints = 8;
        uint32_t PerFaceResolution = 512;
    };

    struct ShadowSettings
    {
        DirectionalShadowSettings Directional;
        SpotShadowSettings Spot;
        PointShadowSettings Point;
    };
}