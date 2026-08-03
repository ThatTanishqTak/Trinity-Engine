#pragma once

#include <memory>

#include <glm/vec2.hpp>

#include <Trinity/Physics/PhysicsTypes.h>
#include <Trinity/Physics/PhysicsSettings.h>
#include <Trinity/Physics/PhysicsEvents.h>
#include <Trinity/Physics/DebugPhysicsDraw.h>

namespace Trinity
{
    class IPhysicsBackend2D;

    class PhysicsWorld2D
    {
    public:
        PhysicsWorld2D();
        ~PhysicsWorld2D();

        PhysicsWorld2D(const PhysicsWorld2D&) = delete;
        PhysicsWorld2D& operator=(const PhysicsWorld2D&) = delete;

        bool Initialize(const PhysicsSettings& settings);
        void Shutdown();

        BodyHandle CreateBody(const BodyDescription2D& description);
        void DestroyBody(BodyHandle body);
        ShapeHandle AddShape(BodyHandle body, const ShapeDescription2D& description);

        void SetBodyTransform(BodyHandle body, const glm::vec2& position, float rotation);
        bool GetBodyTransform(BodyHandle body, glm::vec2& outPosition, float& outRotation) const;

        void SetLinearVelocity(BodyHandle body, const glm::vec2& velocity);
        void ApplyForce(BodyHandle body, const glm::vec2& force);
        void ApplyImpulse(BodyHandle body, const glm::vec2& impulse);

        void Step(float fixedDelta);

        bool Raycast(const glm::vec2& origin, const glm::vec2& direction, float maxDistance, uint32_t layerMask, RaycastHit2D& outHit) const;

        void DrainEvents(PhysicsEventQueue& outEvents);
        void GetDebugLines(DebugDrawBuffer& outBuffer) const;

        bool IsValid() const { return m_Backend != nullptr; }

    private:
        std::unique_ptr<IPhysicsBackend2D> m_Backend;
    };
}