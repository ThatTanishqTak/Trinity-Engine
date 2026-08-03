#pragma once

#include <memory>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Trinity/Physics/PhysicsTypes.h>
#include <Trinity/Physics/PhysicsSettings.h>
#include <Trinity/Physics/PhysicsEvents.h>
#include <Trinity/Physics/DebugPhysicsDraw.h>

namespace Trinity
{
    class IPhysicsBackend3D;

    class PhysicsWorld3D
    {
    public:
        PhysicsWorld3D();
        ~PhysicsWorld3D();

        PhysicsWorld3D(const PhysicsWorld3D&) = delete;
        PhysicsWorld3D& operator=(const PhysicsWorld3D&) = delete;

        bool Initialize(const PhysicsSettings& settings);
        void Shutdown();

        BodyHandle CreateBody(const BodyDescription3D& description);
        void DestroyBody(BodyHandle body);
        ShapeHandle AddShape(BodyHandle body, const ShapeDescription3D& description);

        void SetBodyTransform(BodyHandle body, const glm::vec3& position, const glm::quat& rotation);
        bool GetBodyTransform(BodyHandle body, glm::vec3& outPosition, glm::quat& outRotation) const;

        void SetLinearVelocity(BodyHandle body, const glm::vec3& velocity);
        void ApplyForce(BodyHandle body, const glm::vec3& force);
        void ApplyImpulse(BodyHandle body, const glm::vec3& impulse);

        void Step(float fixedDelta);

        bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, uint32_t layerMask, RaycastHit3D& outHit) const;

        void DrainEvents(PhysicsEventQueue& outEvents);
        void GetDebugLines(DebugDrawBuffer& outBuffer) const;

        bool IsValid() const { return m_Backend != nullptr; }

    private:
        std::unique_ptr<IPhysicsBackend3D> m_Backend;
    };
}