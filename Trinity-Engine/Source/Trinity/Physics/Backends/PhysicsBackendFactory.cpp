#include <Trinity/Physics/Backends/PhysicsBackendFactory.h>

#include <Trinity/Physics/Backends/IPhysicsBackend2D.h>
#include <Trinity/Physics/Backends/IPhysicsBackend3D.h>
#include <Trinity/Core/Log.h>

#if defined(TRINITY_ENABLE_BOX2D)
#include <Trinity/Physics/Backends/Box2D/Box2DBackend.h>
#endif

#if defined(TRINITY_ENABLE_PHYSX)
#include <Trinity/Physics/Backends/PhysX/PhysXBackend.h>
#endif

namespace Trinity
{
    std::unique_ptr<IPhysicsBackend2D> PhysicsBackendFactory::Create2D(PhysicsBackend2D backend)
    {
        switch (backend)
        {
            case PhysicsBackend2D::Box2D:
#if defined(TRINITY_ENABLE_BOX2D)
                TR_CORE_TRACE("2D physics backend selected: Box2D");

                return std::make_unique<Box2DBackend>();
#else
                TR_CORE_TRACE("Box2D backend requested but TRINITY_ENABLE_BOX2D is off");

                return nullptr;
#endif

            default:
                TR_CORE_TRACE("No 2D physics backend selected");

                return nullptr;
        }
    }

    std::unique_ptr<IPhysicsBackend3D> PhysicsBackendFactory::Create3D(PhysicsBackend3D backend)
    {
        switch (backend)
        {
            case PhysicsBackend3D::PhysX:
#if defined(TRINITY_ENABLE_PHYSX)
                TR_CORE_TRACE("3D physics backend selected: PhysX");

                return std::make_unique<PhysXBackend>();
#else
                TR_CORE_TRACE("PhysX backend requested but TRINITY_ENABLE_PHYSX is off");

                return nullptr;
#endif

            default:
                TR_CORE_TRACE("No 3D physics backend selected");

                return nullptr;
        }
    }
}