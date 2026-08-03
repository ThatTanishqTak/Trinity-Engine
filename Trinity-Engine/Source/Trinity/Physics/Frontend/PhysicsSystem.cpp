#include <Trinity/Physics/Frontend/PhysicsSystem.h>

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Trinity/Physics/Frontend/PhysicsWorld2D.h>
#include <Trinity/Physics/Frontend/PhysicsWorld3D.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/IDComponent.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/HierarchyComponent.h>
#include <Trinity/Scene/Components/Rigidbody2DComponent.h>
#include <Trinity/Scene/Components/BoxCollider2DComponent.h>
#include <Trinity/Scene/Components/CircleCollider2DComponent.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    namespace
    {
        constexpr float k_PositionEpsilon = 1.0e-4f;
        constexpr float k_RotationEpsilon = 1.0e-4f;
        constexpr float k_Tau = 6.28318530717958647692f;

        glm::vec2 ExtractWorldPosition2D(const glm::mat4& world)
        {
            return glm::vec2(world[3]);
        }

        // Rotation of the world X axis in the XY plane; valid for the Z-rotation transforms 2D bodies are allowed to have
        float ExtractWorldRotation2D(const glm::mat4& world)
        {
            return std::atan2(world[0][1], world[0][0]);
        }

        glm::vec2 ExtractWorldScale2D(const glm::mat4& world)
        {
            return glm::vec2(glm::length(glm::vec3(world[0])), glm::length(glm::vec3(world[1])));
        }
    }

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
        if (m_ActiveScene != nullptr)
        {
            StopScene(*m_ActiveScene);
        }

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

    void PhysicsSystem::StartScene(Scene& scene)
    {
        if (m_SceneActive)
        {
            return;
        }

        m_Events.Clear();
        m_ActiveScene = &scene;

        if (m_World2D != nullptr)
        {
            // Recreate the world so settings edited since the last session apply
            m_World2D->Shutdown();
            if (!m_World2D->Initialize(m_Settings))
            {
                TR_CORE_ERROR("Failed to reinitialize 2D physics world");
            }

            auto l_View = scene.GetRegistry().view<TransformComponent, Rigidbody2DComponent>();
            for (entt::entity it_Entity : l_View)
            {
                CreateBody2D(scene, it_Entity);
            }

            entt::registry& l_Registry = scene.GetRegistry();
            l_Registry.on_construct<Rigidbody2DComponent>().connect<&PhysicsSystem::OnRigidbody2DConstructed>(*this);
            l_Registry.on_destroy<Rigidbody2DComponent>().connect<&PhysicsSystem::OnRigidbody2DDestroyed>(*this);
            l_Registry.on_construct<BoxCollider2DComponent>().connect<&PhysicsSystem::OnCollider2DChanged>(*this);
            l_Registry.on_destroy<BoxCollider2DComponent>().connect<&PhysicsSystem::OnCollider2DChanged>(*this);
            l_Registry.on_construct<CircleCollider2DComponent>().connect<&PhysicsSystem::OnCollider2DChanged>(*this);
            l_Registry.on_destroy<CircleCollider2DComponent>().connect<&PhysicsSystem::OnCollider2DChanged>(*this);
        }

        m_SceneActive = true;
    }

    void PhysicsSystem::StopScene(Scene& scene)
    {
        entt::registry& l_Registry = scene.GetRegistry();

        if (m_World2D != nullptr)
        {
            l_Registry.on_construct<Rigidbody2DComponent>().disconnect<&PhysicsSystem::OnRigidbody2DConstructed>(*this);
            l_Registry.on_destroy<Rigidbody2DComponent>().disconnect<&PhysicsSystem::OnRigidbody2DDestroyed>(*this);
            l_Registry.on_construct<BoxCollider2DComponent>().disconnect<&PhysicsSystem::OnCollider2DChanged>(*this);
            l_Registry.on_destroy<BoxCollider2DComponent>().disconnect<&PhysicsSystem::OnCollider2DChanged>(*this);
            l_Registry.on_construct<CircleCollider2DComponent>().disconnect<&PhysicsSystem::OnCollider2DChanged>(*this);
            l_Registry.on_destroy<CircleCollider2DComponent>().disconnect<&PhysicsSystem::OnCollider2DChanged>(*this);

            for (auto& it_Body : m_Bodies2D)
            {
                m_World2D->DestroyBody(it_Body.second.Handle);

                if (l_Registry.valid(it_Body.first))
                {
                    if (Rigidbody2DComponent* l_Rigidbody = l_Registry.try_get<Rigidbody2DComponent>(it_Body.first))
                    {
                        l_Rigidbody->Runtime = BodyHandle::Invalid;
                    }
                }
            }
        }

        m_Bodies2D.clear();
        m_PendingRebuilds2D.clear();
        m_Events.Clear();
        m_ActiveScene = nullptr;
        m_SceneActive = false;
    }

    void PhysicsSystem::Step(Scene& scene, float fixedDelta)
    {
        if (!m_SceneActive)
        {
            return;
        }

        // Events accumulate across the frame's sub-steps; Engine::RenderFrame clears them once panels have read them. The cap guards headless callers that never clear
        if (m_Events.Contacts.size() > 4096 || m_Events.Triggers.size() > 4096)
        {
            m_Events.Clear();
        }

        if (m_World2D != nullptr)
        {
            ProcessPendingRebuilds2D(scene);
            SyncSceneToPhysics2D(scene);
            m_World2D->Step(fixedDelta);
            SyncPhysicsToScene2D(scene);
            m_World2D->DrainEvents(m_Events);
        }

        if (m_World3D != nullptr)
        {
            m_World3D->Step(fixedDelta);
            m_World3D->DrainEvents(m_Events);
        }
    }

    void PhysicsSystem::ApplyInterpolation(Scene& scene, float alpha)
    {
        if (!m_SceneActive || m_World2D == nullptr)
        {
            return;
        }

        entt::registry& l_Registry = scene.GetRegistry();
        float l_Alpha = glm::clamp(alpha, 0.0f, 1.0f);

        for (auto& it_Body : m_Bodies2D)
        {
            Body2DRecord& l_Record = it_Body.second;
            if (l_Record.Type != BodyType::Dynamic || !l_Registry.valid(it_Body.first))
            {
                continue;
            }

            glm::vec2 l_Position = glm::mix(l_Record.PreviousPosition, l_Record.CurrentPosition, l_Alpha);

            float l_Delta = std::remainder(l_Record.CurrentRotation - l_Record.PreviousRotation, k_Tau);
            float l_Rotation = l_Record.PreviousRotation + l_Delta * l_Alpha;

            WriteBody2DTransform(scene, it_Body.first, l_Record, l_Position, l_Rotation);
        }
    }

    void PhysicsSystem::CreateBody2D(Scene& scene, entt::entity entity)
    {
        if (m_World2D == nullptr)
        {
            return;
        }

        entt::registry& l_Registry = scene.GetRegistry();
        if (!l_Registry.all_of<TransformComponent, Rigidbody2DComponent>(entity))
        {
            return;
        }

        Rigidbody2DComponent& l_Rigidbody = l_Registry.get<Rigidbody2DComponent>(entity);
        if (l_Rigidbody.Runtime != BodyHandle::Invalid)
        {
            return;
        }

        glm::mat4 l_World = scene.GetWorldMatrix(entity);
        glm::vec2 l_Position = ExtractWorldPosition2D(l_World);
        float l_Rotation = ExtractWorldRotation2D(l_World);
        glm::vec2 l_Scale = ExtractWorldScale2D(l_World);

        BodyDescription2D l_Description;
        l_Description.Type = l_Rigidbody.Type;
        l_Description.Position = l_Position;
        l_Description.Rotation = l_Rotation;
        l_Description.LinearDamping = l_Rigidbody.LinearDamping;
        l_Description.AngularDamping = l_Rigidbody.AngularDamping;
        l_Description.GravityScale = l_Rigidbody.GravityScale;
        l_Description.FixedRotation = l_Rigidbody.FixedRotation;
        l_Description.Layer = l_Rigidbody.Layer;
        l_Description.UserData = static_cast<uint64_t>(l_Registry.get<IDComponent>(entity).ID);

        BodyHandle l_Body = m_World2D->CreateBody(l_Description);
        if (l_Body == BodyHandle::Invalid)
        {
            return;
        }

        // Collider geometry is projected onto the XY physics plane at creation time, so scale, offset, and tilt are all baked; later transform changes require a body rebuild
        auto l_ToBodySpace = [&](const glm::vec2& localPoint) -> glm::vec2
            {
                glm::vec2 l_Projected = glm::vec2(l_World * glm::vec4(localPoint, 0.0f, 1.0f)) - l_Position;
                float l_Sin = std::sin(-l_Rotation);
                float l_Cos = std::cos(-l_Rotation);

                return glm::vec2(l_Projected.x * l_Cos - l_Projected.y * l_Sin, l_Projected.x * l_Sin + l_Projected.y * l_Cos);
            };

        if (const BoxCollider2DComponent* l_Box = l_Registry.try_get<BoxCollider2DComponent>(entity))
        {
            glm::vec2 l_Points[4] = {
                l_ToBodySpace(l_Box->Offset + glm::vec2(-l_Box->HalfExtents.x, -l_Box->HalfExtents.y)),
                l_ToBodySpace(l_Box->Offset + glm::vec2(l_Box->HalfExtents.x, -l_Box->HalfExtents.y)),
                l_ToBodySpace(l_Box->Offset + glm::vec2(l_Box->HalfExtents.x, l_Box->HalfExtents.y)),
                l_ToBodySpace(l_Box->Offset + glm::vec2(-l_Box->HalfExtents.x, l_Box->HalfExtents.y))
            };

            glm::vec2 l_Min = glm::min(glm::min(l_Points[0], l_Points[1]), glm::min(l_Points[2], l_Points[3]));
            glm::vec2 l_Max = glm::max(glm::max(l_Points[0], l_Points[1]), glm::max(l_Points[2], l_Points[3]));
            glm::vec2 l_Extent = (l_Max - l_Min) * 0.5f;
            float l_Area = std::fabs((l_Points[1].x - l_Points[0].x) * (l_Points[3].y - l_Points[0].y) - (l_Points[3].x - l_Points[0].x) * (l_Points[1].y - l_Points[0].y));

            constexpr float k_MinimumHalfExtent = 0.01f;
            if (l_Extent.x < k_MinimumHalfExtent || l_Extent.y < k_MinimumHalfExtent || l_Area < 4.0f * k_MinimumHalfExtent * k_MinimumHalfExtent)
            {
                // Nearly edge-on to the XY plane; use the clamped bounding box instead of a degenerate hull
                glm::vec2 l_Center = (l_Min + l_Max) * 0.5f;
                glm::vec2 l_Clamped = glm::max(l_Extent, glm::vec2(k_MinimumHalfExtent));
                l_Points[0] = l_Center + glm::vec2(-l_Clamped.x, -l_Clamped.y);
                l_Points[1] = l_Center + glm::vec2(l_Clamped.x, -l_Clamped.y);
                l_Points[2] = l_Center + glm::vec2(l_Clamped.x, l_Clamped.y);
                l_Points[3] = l_Center + glm::vec2(-l_Clamped.x, l_Clamped.y);

                TR_CORE_WARN("2D box collider is nearly edge-on to the XY plane; clamped to minimum thickness");
            }

            ShapeDescription2D l_Shape;
            l_Shape.Type = ShapeType2D::Polygon;
            for (int l_Index = 0; l_Index < 4; ++l_Index)
            {
                l_Shape.Points[l_Index] = l_Points[l_Index];
            }
            l_Shape.PointCount = 4;
            l_Shape.IsTrigger = l_Box->IsTrigger;
            l_Shape.Material = l_Box->Material;
            m_World2D->AddShape(l_Body, l_Shape);
        }

        if (const CircleCollider2DComponent* l_Circle = l_Registry.try_get<CircleCollider2DComponent>(entity))
        {
            ShapeDescription2D l_Shape;
            l_Shape.Type = ShapeType2D::Circle;
            l_Shape.Offset = l_ToBodySpace(l_Circle->Offset);
            l_Shape.Radius = glm::max(l_Circle->Radius * glm::max(glm::length(glm::vec2(l_World[0])), glm::length(glm::vec2(l_World[1]))), 0.01f);
            l_Shape.IsTrigger = l_Circle->IsTrigger;
            l_Shape.Material = l_Circle->Material;
            m_World2D->AddShape(l_Body, l_Shape);
        }

        l_Rigidbody.Runtime = l_Body;

        Body2DRecord l_Record;
        l_Record.Handle = l_Body;
        l_Record.Type = l_Rigidbody.Type;
        l_Record.PreviousPosition = l_Record.CurrentPosition = l_Record.LastWrittenPosition = l_Position;
        l_Record.PreviousRotation = l_Record.CurrentRotation = l_Record.LastWrittenRotation = l_Rotation;
        l_Record.ScaleAtCreation = l_Scale;

        glm::vec3 l_Normal(l_World[2]);
        float l_NormalLength = glm::length(l_Normal);
        if (l_NormalLength > 1.0e-6f)
        {
            l_Record.PlaneNormalAtCreation = l_Normal / l_NormalLength;
        }

        m_Bodies2D[entity] = l_Record;
    }

    void PhysicsSystem::DestroyBody2D(entt::registry& registry, entt::entity entity)
    {
        auto l_Found = m_Bodies2D.find(entity);
        if (l_Found == m_Bodies2D.end())
        {
            return;
        }

        if (m_World2D != nullptr)
        {
            m_World2D->DestroyBody(l_Found->second.Handle);
        }

        m_Bodies2D.erase(l_Found);

        if (registry.valid(entity))
        {
            if (Rigidbody2DComponent* l_Rigidbody = registry.try_get<Rigidbody2DComponent>(entity))
            {
                l_Rigidbody->Runtime = BodyHandle::Invalid;
            }
        }
    }

    void PhysicsSystem::ProcessPendingRebuilds2D(Scene& scene)
    {
        if (m_PendingRebuilds2D.empty())
        {
            return;
        }

        entt::registry& l_Registry = scene.GetRegistry();
        for (entt::entity it_Entity : m_PendingRebuilds2D)
        {
            DestroyBody2D(l_Registry, it_Entity);

            if (l_Registry.valid(it_Entity) && l_Registry.all_of<TransformComponent, Rigidbody2DComponent>(it_Entity))
            {
                CreateBody2D(scene, it_Entity);
            }
        }

        m_PendingRebuilds2D.clear();
    }

    void PhysicsSystem::SyncSceneToPhysics2D(Scene& scene)
    {
        entt::registry& l_Registry = scene.GetRegistry();

        for (auto& it_Body : m_Bodies2D)
        {
            if (!l_Registry.valid(it_Body.first))
            {
                continue;
            }

            Body2DRecord& l_Record = it_Body.second;

            glm::mat4 l_World = scene.GetWorldMatrix(it_Body.first);
            glm::vec2 l_Position = ExtractWorldPosition2D(l_World);
            float l_Rotation = ExtractWorldRotation2D(l_World);

            bool l_Dirty = glm::length(l_Position - l_Record.LastWrittenPosition) > k_PositionEpsilon
                || std::fabs(std::remainder(l_Rotation - l_Record.LastWrittenRotation, k_Tau)) > k_RotationEpsilon;

            if (l_Record.Type == BodyType::Kinematic || l_Dirty)
            {
                m_World2D->SetBodyTransform(l_Record.Handle, l_Position, l_Rotation);
                l_Record.PreviousPosition = l_Record.CurrentPosition = l_Record.LastWrittenPosition = l_Position;
                l_Record.PreviousRotation = l_Record.CurrentRotation = l_Record.LastWrittenRotation = l_Rotation;
            }

            if (!l_Record.ScaleWarned)
            {
                glm::vec2 l_Scale = ExtractWorldScale2D(l_World);
                if (glm::length(l_Scale - l_Record.ScaleAtCreation) > 1.0e-3f)
                {
                    TR_CORE_WARN("Entity scale changed during play; 2D collider extents keep their creation-time size until the body is rebuilt");
                    l_Record.ScaleWarned = true;
                }
            }

            if (!l_Record.TiltWarned)
            {
                glm::vec3 l_Normal(l_World[2]);
                float l_NormalLength = glm::length(l_Normal);
                if (l_NormalLength > 1.0e-6f && glm::dot(l_Normal / l_NormalLength, l_Record.PlaneNormalAtCreation) < 0.999f)
                {
                    TR_CORE_WARN("Entity tilt changed during play; the 2D collider projection keeps its creation-time shape until the body is rebuilt");
                    l_Record.TiltWarned = true;
                }
            }
        }
    }

    void PhysicsSystem::SyncPhysicsToScene2D(Scene& scene)
    {
        entt::registry& l_Registry = scene.GetRegistry();

        for (auto& it_Body : m_Bodies2D)
        {
            Body2DRecord& l_Record = it_Body.second;
            if (l_Record.Type != BodyType::Dynamic || !l_Registry.valid(it_Body.first))
            {
                continue;
            }

            glm::vec2 l_Position;
            float l_Rotation = 0.0f;
            if (!m_World2D->GetBodyTransform(l_Record.Handle, l_Position, l_Rotation))
            {
                continue;
            }

            l_Record.PreviousPosition = l_Record.CurrentPosition;
            l_Record.PreviousRotation = l_Record.CurrentRotation;
            l_Record.CurrentPosition = l_Position;
            l_Record.CurrentRotation = l_Rotation;

            WriteBody2DTransform(scene, it_Body.first, l_Record, l_Position, l_Rotation);
        }
    }

    void PhysicsSystem::WriteBody2DTransform(Scene& scene, entt::entity entity, Body2DRecord& record, const glm::vec2& position, float rotation)
    {
        entt::registry& l_Registry = scene.GetRegistry();
        TransformComponent* l_Transform = l_Registry.try_get<TransformComponent>(entity);
        if (l_Transform == nullptr)
        {
            return;
        }

        glm::mat4 l_World = scene.GetWorldMatrix(entity);
        glm::vec3 l_WorldScale(glm::max(glm::length(glm::vec3(l_World[0])), 1.0e-6f), glm::max(glm::length(glm::vec3(l_World[1])), 1.0e-6f), glm::max(glm::length(glm::vec3(l_World[2])), 1.0e-6f));

        // Physics owns world XY position and the twist about Z; authored tilt (the swing), world Z translation, and scale are preserved
        glm::mat3 l_WorldAxes(glm::vec3(l_World[0]) / l_WorldScale.x, glm::vec3(l_World[1]) / l_WorldScale.y, glm::vec3(l_World[2]) / l_WorldScale.z);
        glm::quat l_Current = glm::normalize(glm::quat_cast(l_WorldAxes));

        glm::quat l_Twist{ 1.0f, 0.0f, 0.0f, 0.0f };
        if (std::fabs(l_Current.w) > 1.0e-6f || std::fabs(l_Current.z) > 1.0e-6f)
        {
            l_Twist = glm::normalize(glm::quat(l_Current.w, 0.0f, 0.0f, l_Current.z));
        }
        glm::quat l_Swing = l_Current * glm::inverse(l_Twist);

        glm::mat4 l_NewWorld = glm::translate(glm::mat4(1.0f), glm::vec3(position, l_World[3].z)) * glm::mat4_cast(l_Swing * glm::angleAxis(rotation, glm::vec3(0.0f, 0.0f, 1.0f))) * glm::scale(glm::mat4(1.0f), l_WorldScale);

        glm::mat4 l_Local = l_NewWorld;
        if (const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(entity))
        {
            if (l_Hierarchy->Parent != entt::null)
            {
                l_Local = glm::inverse(scene.GetWorldMatrix(l_Hierarchy->Parent)) * l_NewWorld;
            }
        }

        glm::vec3 l_Scale(glm::max(glm::length(glm::vec3(l_Local[0])), 1.0e-6f), glm::max(glm::length(glm::vec3(l_Local[1])), 1.0e-6f), glm::max(glm::length(glm::vec3(l_Local[2])), 1.0e-6f));
        glm::mat3 l_RotationMatrix(glm::vec3(l_Local[0]) / l_Scale.x, glm::vec3(l_Local[1]) / l_Scale.y, glm::vec3(l_Local[2]) / l_Scale.z);

        l_Transform->Translation = glm::vec3(l_Local[3]);
        l_Transform->Rotation = glm::normalize(glm::quat_cast(l_RotationMatrix));
        l_Transform->Scale = l_Scale;

        // Under tilt the extracted Z angle is not exactly the body angle, so dirty detection must compare against what was actually written
        record.LastWrittenPosition = ExtractWorldPosition2D(l_NewWorld);
        record.LastWrittenRotation = ExtractWorldRotation2D(l_NewWorld);
    }

    void PhysicsSystem::OnRigidbody2DConstructed(entt::registry&, entt::entity entity)
    {
        if (m_ActiveScene != nullptr)
        {
            CreateBody2D(*m_ActiveScene, entity);
        }
    }

    void PhysicsSystem::OnRigidbody2DDestroyed(entt::registry& registry, entt::entity entity)
    {
        DestroyBody2D(registry, entity);
    }

    void PhysicsSystem::OnCollider2DChanged(entt::registry&, entt::entity entity)
    {
        if (m_ActiveScene == nullptr)
        {
            return;
        }

        // Deferred: at callback time the collider component is mid add/remove, so the body is rebuilt at the start of the next Step
        if (std::find(m_PendingRebuilds2D.begin(), m_PendingRebuilds2D.end(), entity) == m_PendingRebuilds2D.end())
        {
            m_PendingRebuilds2D.push_back(entity);
        }
    }
}