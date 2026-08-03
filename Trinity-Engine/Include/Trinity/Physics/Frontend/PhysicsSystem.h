#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

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

        // Writes lerp(previous, current) into simulated transforms; call once per rendered frame with the simulation clock's alpha.
        void ApplyInterpolation(Scene& scene, float alpha);

        PhysicsSettings& GetSettings() { return m_Settings; }
        const PhysicsSettings& GetSettings() const { return m_Settings; }

        PhysicsWorld2D* GetWorld2D() { return m_World2D.get(); }
        PhysicsWorld3D* GetWorld3D() { return m_World3D.get(); }
        bool HasWorld2D() const { return m_World2D != nullptr; }
        bool HasWorld3D() const { return m_World3D != nullptr; }

        const PhysicsEventQueue& GetEvents() const { return m_Events; }
        void ClearEvents() { m_Events.Clear(); }
        bool IsSceneActive() const { return m_SceneActive; }

    private:
        struct Body2DRecord
        {
            BodyHandle Handle = BodyHandle::Invalid;
            BodyType Type = BodyType::Static;
            glm::vec2 PreviousPosition{ 0.0f };
            glm::vec2 CurrentPosition{ 0.0f };
            glm::vec2 LastWrittenPosition{ 0.0f };
            float PreviousRotation = 0.0f;
            float CurrentRotation = 0.0f;
            float LastWrittenRotation = 0.0f;
            glm::vec2 ScaleAtCreation{ 1.0f };
            bool ScaleWarned = false;
            glm::vec3 PlaneNormalAtCreation{ 0.0f, 0.0f, 1.0f };
            bool TiltWarned = false;
        };

        void CreateBody2D(Scene& scene, entt::entity entity);
        void DestroyBody2D(entt::registry& registry, entt::entity entity);
        void ProcessPendingRebuilds2D(Scene& scene);
        void SyncSceneToPhysics2D(Scene& scene);
        void SyncPhysicsToScene2D(Scene& scene);
        void WriteBody2DTransform(Scene& scene, entt::entity entity, Body2DRecord& record, const glm::vec2& position, float rotation);

        void OnRigidbody2DConstructed(entt::registry& registry, entt::entity entity);
        void OnRigidbody2DDestroyed(entt::registry& registry, entt::entity entity);
        void OnCollider2DChanged(entt::registry& registry, entt::entity entity);

        PhysicsSettings m_Settings;
        std::unique_ptr<PhysicsWorld2D> m_World2D;
        std::unique_ptr<PhysicsWorld3D> m_World3D;
        PhysicsEventQueue m_Events;

        Scene* m_ActiveScene = nullptr;
        std::unordered_map<entt::entity, Body2DRecord> m_Bodies2D;
        std::vector<entt::entity> m_PendingRebuilds2D;
        bool m_SceneActive = false;
    };
}