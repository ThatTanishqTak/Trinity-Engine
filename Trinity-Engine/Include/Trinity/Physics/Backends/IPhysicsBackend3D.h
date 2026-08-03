#pragma once

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Trinity/Physics/PhysicsTypes.h>
#include <Trinity/Physics/PhysicsSettings.h>
#include <Trinity/Physics/PhysicsEvents.h>
#include <Trinity/Physics/DebugPhysicsDraw.h>

namespace Trinity
{
    class IPhysicsBackend3D
    {
    public:
        virtual ~IPhysicsBackend3D() = default;

        virtual bool Initialize(const PhysicsSettings& settings) = 0;
        virtual void Shutdown() = 0;

        virtual BodyHandle CreateBody(const BodyDescription3D& description) = 0;
        virtual void DestroyBody(BodyHandle body) = 0;
        virtual ShapeHandle AddShape(BodyHandle body, const ShapeDescription3D& description) = 0;

        virtual void SetBodyTransform(BodyHandle body, const glm::vec3& position, const glm::quat& rotation) = 0;
        virtual bool GetBodyTransform(BodyHandle body, glm::vec3& outPosition, glm::quat& outRotation) const = 0;

        virtual void SetLinearVelocity(BodyHandle body, const glm::vec3& velocity) = 0;
        virtual void ApplyForce(BodyHandle body, const glm::vec3& force) = 0;
        virtual void ApplyImpulse(BodyHandle body, const glm::vec3& impulse) = 0;

        virtual void Step(float fixedDelta) = 0;

        virtual bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, uint32_t layerMask, RaycastHit3D& outHit) const = 0;

        virtual void DrainEvents(PhysicsEventQueue& outEvents) = 0;
        virtual void GetDebugLines(DebugDrawBuffer& outBuffer) const = 0;
    };
}