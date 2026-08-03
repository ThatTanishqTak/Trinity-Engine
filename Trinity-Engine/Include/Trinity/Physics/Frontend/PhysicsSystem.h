#pragma once

#include <memory>

#include <Trinity/Physics/PhysicsTypes.h>
#include <Trinity/Physics/PhysicsSettings.h>
#include <Trinity/Physics/PhysicsEvents.h>

namespace Trinity
{
    class Scene;
    class PhysicsWorld2D;
    class PhysicsWorld3D;

    class PhysicsSystem
    {
    public:
        PhysicsSystem();
        ~PhysicsSystem();

        PhysicsSystem(const PhysicsSystem&) = delete;
        PhysicsSystem& operator=(const PhysicsSystem&) = delete;

        bool Initialize(const PhysicsSettings& settings = PhysicsSettings{});
        void Shutdown();

        void StartScene(Scene& scene);
        void StopScene(Scene& scene);

        void Step(Scene& scene, float fixedDelta);

        PhysicsSettings& GetSettings() { return m_Settings; }
        const PhysicsSettings& GetSettings() const { return m_Settings; }

        PhysicsWorld2D* GetWorld2D() { return m_World2D.get(); }
        PhysicsWorld3D* GetWorld3D() { return m_World3D.get(); }
        bool HasWorld2D() const { return m_World2D != nullptr; }
        bool HasWorld3D() const { return m_World3D != nullptr; }

        const PhysicsEventQueue& GetEvents() const { return m_Events; }
        bool IsSceneActive() const { return m_SceneActive; }

    private:
        PhysicsSettings m_Settings;
        std::unique_ptr<PhysicsWorld2D> m_World2D;
        std::unique_ptr<PhysicsWorld3D> m_World3D;
        PhysicsEventQueue m_Events;
        bool m_SceneActive = false;
    };
}