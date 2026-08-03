#include <Trinity/Physics/Frontend/PhysicsWorld3D.h>

#include <Trinity/Physics/Backends/IPhysicsBackend3D.h>
#include <Trinity/Physics/Backends/PhysicsBackendFactory.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    PhysicsWorld3D::PhysicsWorld3D() = default;

    PhysicsWorld3D::~PhysicsWorld3D()
    {
        Shutdown();
    }

    bool PhysicsWorld3D::Initialize(const PhysicsSettings& settings)
    {
        m_Backend = PhysicsBackendFactory::Create3D(settings.Backend3D);
        if (m_Backend == nullptr)
        {
            return false;
        }

        if (!m_Backend->Initialize(settings))
        {
            TR_CORE_ERROR("Failed to initialize 3D physics backend");
            m_Backend.reset();

            return false;
        }

        return true;
    }

    void PhysicsWorld3D::Shutdown()
    {
        if (m_Backend != nullptr)
        {
            m_Backend->Shutdown();
            m_Backend.reset();
        }
    }

    BodyHandle PhysicsWorld3D::CreateBody(const BodyDescription3D& description)
    {
        return m_Backend != nullptr ? m_Backend->CreateBody(description) : BodyHandle::Invalid;
    }

    void PhysicsWorld3D::DestroyBody(BodyHandle body)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->DestroyBody(body);
        }
    }

    ShapeHandle PhysicsWorld3D::AddShape(BodyHandle body, const ShapeDescription3D& description)
    {
        return m_Backend != nullptr ? m_Backend->AddShape(body, description) : ShapeHandle::Invalid;
    }

    void PhysicsWorld3D::SetBodyTransform(BodyHandle body, const glm::vec3& position, const glm::quat& rotation)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->SetBodyTransform(body, position, rotation);
        }
    }

    bool PhysicsWorld3D::GetBodyTransform(BodyHandle body, glm::vec3& outPosition, glm::quat& outRotation) const
    {
        return m_Backend != nullptr && m_Backend->GetBodyTransform(body, outPosition, outRotation);
    }

    void PhysicsWorld3D::SetLinearVelocity(BodyHandle body, const glm::vec3& velocity)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->SetLinearVelocity(body, velocity);
        }
    }

    void PhysicsWorld3D::ApplyForce(BodyHandle body, const glm::vec3& force)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->ApplyForce(body, force);
        }
    }

    void PhysicsWorld3D::ApplyImpulse(BodyHandle body, const glm::vec3& impulse)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->ApplyImpulse(body, impulse);
        }
    }

    void PhysicsWorld3D::Step(float fixedDelta)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->Step(fixedDelta);
        }
    }

    bool PhysicsWorld3D::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, uint32_t layerMask, RaycastHit3D& outHit) const
    {
        return m_Backend != nullptr && m_Backend->Raycast(origin, direction, maxDistance, layerMask, outHit);
    }

    void PhysicsWorld3D::DrainEvents(PhysicsEventQueue& outEvents)
    {
        if (m_Backend != nullptr)
        {
            m_Backend->DrainEvents(outEvents);
        }
    }

    void PhysicsWorld3D::GetDebugLines(DebugDrawBuffer& outBuffer) const
    {
        if (m_Backend != nullptr)
        {
            m_Backend->GetDebugLines(outBuffer);
        }
    }
}