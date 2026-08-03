#pragma once

#include <cstdint>

#include <Trinity/Physics/PhysicsTypes.h>

namespace Trinity
{
    struct Rigidbody2DComponent
    {
        BodyType Type = BodyType::Dynamic;
        bool FixedRotation = false;
        float GravityScale = 1.0f;
        float LinearDamping = 0.0f;
        float AngularDamping = 0.05f;
        uint32_t Layer = 0;

        BodyHandle Runtime = BodyHandle::Invalid; // live body while playing; never serialized
    };
}