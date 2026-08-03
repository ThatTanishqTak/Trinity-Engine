#include <Trinity/Physics/Frontend/PhysicsSystem.h>

#include <Trinity/Physics/Frontend/PhysicsWorld2D.h>
#include <Trinity/Physics/Frontend/PhysicsWorld3D.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    PhysicsSystem::PhysicsSystem() = default;

    PhysicsSystem::~PhysicsSystem()
    {
        Shutdown();
    }

    bool PhysicsSystem::Initialize(const PhysicsSettings& settings)
    {
        TR_CORE_INFO("INITIALIZING PHYSICS");

        m_Settings = settings;

        auto l_World2D = std::make_unique<PhysicsWorld2D>();
        if (l_World2D->Initialize(m_Settings))
        {
            m_World2D = std::move(l_World2D);
        }
        else
        {
            TR_CORE_INFO("2D physics world unavailable (no backend compiled in)");
        }

        auto l_World3D = std::make_unique<PhysicsWorld3D>();
        if (l_World3D->Initialize(m_Settings))
        {
            m_World3D = std::move(l_World3D);
        }
        else
        {
            TR_CORE_INFO("3D physics world unavailable (no backend compiled in)");
        }

        TR_CORE_INFO("PHYSICS INITIALIZED");

        return true;
    }

    void PhysicsSystem::Shutdown()
    {
        if (m_World2D != nullptr)
        {
            m_World2D->Shutdown();
            m_World2D.reset();
        }

        if (m_World3D != nullptr)
        {
            m_World3D->Shutdown();
            m_World3D.reset();
        }

        m_Events.Clear();
        m_SceneActive = false;
    }

    void PhysicsSystem::StartScene(Scene&)
    {
        m_Events.Clear();
        m_SceneActive = true;
    }

    void PhysicsSystem::StopScene(Scene&)
    {
        m_Events.Clear();
        m_SceneActive = false;
    }

    void PhysicsSystem::Step(Scene&, float fixedDelta)
    {
        if (!m_SceneActive)
        {
            return;
        }

        m_Events.Clear();

        if (m_World2D != nullptr)
        {
            m_World2D->Step(fixedDelta);
            m_World2D->DrainEvents(m_Events);
        }

        if (m_World3D != nullptr)
        {
            m_World3D->Step(fixedDelta);
            m_World3D->DrainEvents(m_Events);
        }
    }
}