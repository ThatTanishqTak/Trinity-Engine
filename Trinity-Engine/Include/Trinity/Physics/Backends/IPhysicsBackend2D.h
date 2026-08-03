#pragma once

#include <glm/vec2.hpp>

#include <Trinity/Physics/PhysicsTypes.h>
#include <Trinity/Physics/PhysicsSettings.h>
#include <Trinity/Physics/PhysicsEvents.h>
#include <Trinity/Physics/DebugPhysicsDraw.h>

namespace Trinity
{
    // 2D plane convention: XY plane, +Z toward the viewer, rotation about +Z, 1 world unit = 1 metre. No pixels-per-metre scaling anywhere
    class IPhysicsBackend2D
    {
    public:
        virtual ~IPhysicsBackend2D() = default;

        virtual bool Initialize(const PhysicsSettings& settings) = 0;
        virtual void Shutdown() = 0;

        virtual BodyHandle CreateBody(const BodyDescription2D& description) = 0;
        virtual void DestroyBody(BodyHandle body) = 0;
        virtual ShapeHandle AddShape(BodyHandle body, const ShapeDescription2D& description) = 0;

        virtual void SetBodyTransform(BodyHandle body, const glm::vec2& position, float rotation) = 0;
        virtual bool GetBodyTransform(BodyHandle body, glm::vec2& outPosition, float& outRotation) const = 0;

        virtual void SetLinearVelocity(BodyHandle body, const glm::vec2& velocity) = 0;
        virtual void ApplyForce(BodyHandle body, const glm::vec2& force) = 0;
        virtual void ApplyImpulse(BodyHandle body, const glm::vec2& impulse) = 0;

        virtual void Step(float fixedDelta) = 0;

        virtual bool Raycast(const glm::vec2& origin, const glm::vec2& direction, float maxDistance, uint32_t layerMask, RaycastHit2D& outHit) const = 0;

        virtual void DrainEvents(PhysicsEventQueue& outEvents) = 0;
        virtual void GetDebugLines(DebugDrawBuffer& outBuffer) const = 0;
    };
}