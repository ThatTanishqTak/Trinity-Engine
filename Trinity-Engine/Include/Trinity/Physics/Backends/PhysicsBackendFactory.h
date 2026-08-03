#pragma once

#include <memory>

#include <Trinity/Physics/PhysicsTypes.h>

namespace Trinity
{
    class IPhysicsBackend2D;
    class IPhysicsBackend3D;

    class PhysicsBackendFactory
    {
    public:
        static std::unique_ptr<IPhysicsBackend2D> Create2D(PhysicsBackend2D backend);
        static std::unique_ptr<IPhysicsBackend3D> Create3D(PhysicsBackend3D backend);
    };
}