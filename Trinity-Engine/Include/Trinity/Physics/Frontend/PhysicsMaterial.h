#pragma once

#include <cstdint>

namespace Trinity
{
    enum class PhysicsCombineMode : uint32_t
    {
        Average = 0,
        Minimum,
        Multiply,
        Maximum
    };

    struct PhysicsMaterial
    {
        float Friction = 0.5f;
        float Restitution = 0.0f;
        float Density = 1.0f;

        PhysicsCombineMode FrictionCombine = PhysicsCombineMode::Average;
        PhysicsCombineMode RestitutionCombine = PhysicsCombineMode::Average;
    };
}