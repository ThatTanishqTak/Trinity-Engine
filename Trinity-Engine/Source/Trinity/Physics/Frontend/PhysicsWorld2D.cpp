#include <Trinity/Physics/Frontend/PhysicsWorld2D.h>

#include <Trinity/Physics/Backends/IPhysicsBackend2D.h>
#include <Trinity/Physics/Backends/PhysicsBackendFactory.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    PhysicsWorld2D::PhysicsWorld2D() = default;

    PhysicsWorld2D::~PhysicsWorld2D()
    {
        Shutdown();
    }

    bool PhysicsWorld2D::Initialize(const PhysicsSettings& settings)
    {
        m_Backend = PhysicsBackendFactory::Create2D(settings.Backend2D);
        if (m_Backend == nullptr)
        {
            return false;
        }

        if (!m_Backend->Initialize(settings))
        {
            TR_CORE_ERROR("Failed to initialize 2D physics backend");
            m_Backend.reset();

            return false;
        }

        return true;
    }

    void PhysicsWorld2D::Shutdown()
    {
        if (m_Backend != nullptr)
        {
            m_Backend->Shutdown();
            m_Backend.reset();
        }
    }

    BodyHandle PhysicsWorld2D::CreateBody(const BodyDescription2D& description)
    {
        return m_Backend != nullptr ? m_Backend->CreateBody(description) : BodyHandle::Invalid;
    }

    void PhysicsWorld2D::DestroyBody(BodyHandle body)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->DestroyBody(body);
        }
    }

    ShapeHandle PhysicsWorld2D::AddShape(BodyHandle body, const ShapeDescription2D& description)
    {
        return m_Backend != nullptr ? m_Backend->AddShape(body, description) : ShapeHandle::Invalid;
    }

    void PhysicsWorld2D::SetBodyTransform(BodyHandle body, const glm::vec2& position, float rotation)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->SetBodyTransform(body, position, rotation);
        }
    }

    bool PhysicsWorld2D::GetBodyTransform(BodyHandle body, glm::vec2& outPosition, float& outRotation) const
    {
        return m_Backend != nullptr && m_Backend->GetBodyTransform(body, outPosition, outRotation);
    }

    bool PhysicsWorld2D::IsBodyAwake(BodyHandle body) const
    {
        return m_Backend != nullptr && m_Backend->IsBodyAwake(body);
    }

    void PhysicsWorld2D::SetLinearVelocity(BodyHandle body, const glm::vec2& velocity)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->SetLinearVelocity(body, velocity);
        }
    }

    void PhysicsWorld2D::ApplyForce(BodyHandle body, const glm::vec2& force)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->ApplyForce(body, force);
        }
    }

    void PhysicsWorld2D::ApplyImpulse(BodyHandle body, const glm::vec2& impulse)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->ApplyImpulse(body, impulse);
        }
    }

    void PhysicsWorld2D::Step(float fixedDelta)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->Step(fixedDelta);
        }
    }

    bool PhysicsWorld2D::Raycast(const glm::vec2& origin, const glm::vec2& direction, float maxDistance, uint32_t layerMask, RaycastHit2D& outHit) const
    {
        return m_Backend != nullptr && m_Backend->Raycast(origin, direction, maxDistance, layerMask, outHit);
    }

    void PhysicsWorld2D::DrainEvents(PhysicsEventQueue& outEvents)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->DrainEvents(outEvents);
        }
    }

    void PhysicsWorld2D::GetDebugLines(DebugDrawBuffer& outBuffer) const
    {
        if (m_Backend != nullptr)
        {
            m_Backend->GetDebugLines(outBuffer);
        }
    }
}