#pragma once

#include <memory>

#include <Trinity/Physics/Backends/IPhysicsBackend2D.h>

namespace Trinity
{
    // Box2D v3 (C API) implementation; all box2d types live in the .cpp.
    class Box2DBackend : public IPhysicsBackend2D
    {
    public:
        Box2DBackend();
        ~Box2DBackend() override;

        Box2DBackend(const Box2DBackend&) = delete;
        Box2DBackend& operator=(const Box2DBackend&) = delete;

        bool Initialize(const PhysicsSettings& settings) override;
        void Shutdown() override;

        BodyHandle CreateBody(const BodyDescription2D& description) override;
        void DestroyBody(BodyHandle body) override;
        ShapeHandle AddShape(BodyHandle body, const ShapeDescription2D& description) override;

        void SetBodyTransform(BodyHandle body, const glm::vec2& position, float rotation) override;
        bool GetBodyTransform(BodyHandle body, glm::vec2& outPosition, float& outRotation) const override;
        bool IsBodyAwake(BodyHandle body) const override;

        void SetLinearVelocity(BodyHandle body, const glm::vec2& velocity) override;
        void ApplyForce(BodyHandle body, const glm::vec2& force) override;
        void ApplyImpulse(BodyHandle body, const glm::vec2& impulse) override;

        void Step(float fixedDelta) override;

        bool Raycast(const glm::vec2& origin, const glm::vec2& direction, float maxDistance, uint32_t layerMask, RaycastHit2D& outHit) const override;

        void DrainEvents(PhysicsEventQueue& outEvents) override;
        void GetDebugLines(DebugDrawBuffer& outBuffer) const override;

    private:
        struct Implementation;
        std::unique_ptr<Implementation> m_Implementation;
    };
}