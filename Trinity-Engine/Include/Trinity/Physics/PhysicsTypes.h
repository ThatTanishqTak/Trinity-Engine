#pragma once

#include <cstdint>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Trinity/Core/UUID.h>
#include <Trinity/Physics/Frontend/PhysicsMaterial.h>

namespace Trinity
{
    enum class PhysicsBackend2D : uint32_t
    {
        Box2D = 0
    };

    enum class PhysicsBackend3D : uint32_t
    {
        PhysX = 0
    };

    enum class BodyHandle : uint64_t
    {
        Invalid = 0
    };

    enum class ShapeHandle : uint64_t
    {
        Invalid = 0
    };

    enum class BodyType : uint32_t
    {
        Static = 0,
        Kinematic,
        Dynamic
    };

    enum class ShapeType2D : uint32_t
    {
        Box = 0,
        Circle,
        Polygon
    };

    enum class ShapeType3D : uint32_t
    {
        Box = 0,
        Sphere,
        Capsule
    };

    struct BodyDescription2D
    {
        BodyType Type = BodyType::Static;
        glm::vec2 Position{ 0.0f };
        float Rotation = 0.0f;            // radians about +Z
        float LinearDamping = 0.0f;
        float AngularDamping = 0.0f;
        float GravityScale = 1.0f;
        bool FixedRotation = false;
        uint32_t Layer = 0;
        uint64_t UserData = 0;            // entity UUID, echoed back in events and hits
    };

    struct ShapeDescription2D
    {
        ShapeType2D Type = ShapeType2D::Box;
        glm::vec2 Offset{ 0.0f };
        glm::vec2 HalfExtents{ 0.5f };    // Box
        float Radius = 0.5f;              // Circle
        glm::vec2 Points[8] = {};         // Polygon: convex points in body space
        uint32_t PointCount = 0;          // Polygon
        bool IsTrigger = false;
        PhysicsMaterial Material;
    };

    struct BodyDescription3D
    {
        BodyType Type = BodyType::Static;
        glm::vec3 Position{ 0.0f };
        glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        float Mass = 1.0f;
        float LinearDamping = 0.0f;
        float AngularDamping = 0.05f;
        bool UseGravity = true;
        uint32_t Layer = 0;
        uint64_t UserData = 0;            // entity UUID, echoed back in events and hits
    };

    struct ShapeDescription3D
    {
        ShapeType3D Type = ShapeType3D::Box;
        glm::vec3 Offset{ 0.0f };
        glm::vec3 HalfExtents{ 0.5f };    // Box
        float Radius = 0.5f;              // Sphere / Capsule
        float HalfHeight = 0.5f;          // Capsule, excluding the hemispherical caps
        bool IsTrigger = false;
        PhysicsMaterial Material;
    };

    struct RaycastHit2D
    {
        UUID Entity = UUID(0);
        glm::vec2 Point{ 0.0f };
        glm::vec2 Normal{ 0.0f };
        float Distance = 0.0f;
    };

    struct RaycastHit3D
    {
        UUID Entity = UUID(0);
        glm::vec3 Point{ 0.0f };
        glm::vec3 Normal{ 0.0f };
        float Distance = 0.0f;
    };
}