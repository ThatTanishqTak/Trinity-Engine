#include <Trinity/Physics/Backends/Box2D/Box2DBackend.h>

#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <glm/geometric.hpp>

#include <box2d/box2d.h>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    namespace
    {
        int Box2DAssertCallback(const char* condition, const char* fileName, int lineNumber)
        {
            TR_CORE_ERROR("Box2D assertion failed: {} ({}:{})", condition, fileName, lineNumber);

            return 1;
        }

        uint64_t ReadBodyUUID(b2BodyId body)
        {
            return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(body)));
        }

        uint32_t PackColor(b2HexColor color)
        {
            uint32_t l_Red = (static_cast<uint32_t>(color) >> 16) & 0xFF;
            uint32_t l_Green = (static_cast<uint32_t>(color) >> 8) & 0xFF;
            uint32_t l_Blue = static_cast<uint32_t>(color) & 0xFF;

            return l_Red | (l_Green << 8) | (l_Blue << 16) | 0xFF000000u;
        }

        void AppendLine(DebugDrawBuffer& buffer, b2Vec2 start, b2Vec2 end, b2HexColor color)
        {
            buffer.AddLine(glm::vec3(start.x, start.y, 0.0f), glm::vec3(end.x, end.y, 0.0f), PackColor(color));
        }

        void DrawCircleSegments(DebugDrawBuffer& buffer, b2Vec2 center, float radius, b2HexColor color)
        {
            constexpr int k_Segments = 16;
            constexpr float k_Tau = 6.28318530718f;

            b2Vec2 l_Previous = { center.x + radius, center.y };
            for (int l_Index = 1; l_Index <= k_Segments; ++l_Index)
            {
                float l_Angle = k_Tau * static_cast<float>(l_Index) / static_cast<float>(k_Segments);
                b2Vec2 l_Point = { center.x + radius * std::cos(l_Angle), center.y + radius * std::sin(l_Angle) };
                AppendLine(buffer, l_Previous, l_Point, color);
                l_Previous = l_Point;
            }
        }

        void DrawPolygonCallback(const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context)
        {
            DebugDrawBuffer& l_Buffer = *static_cast<DebugDrawBuffer*>(context);
            for (int l_Index = 0; l_Index < vertexCount; ++l_Index)
            {
                AppendLine(l_Buffer, vertices[l_Index], vertices[(l_Index + 1) % vertexCount], color);
            }
        }

        void DrawSolidPolygonCallback(b2Transform transform, const b2Vec2* vertices, int vertexCount, float, b2HexColor color, void* context)
        {
            DebugDrawBuffer& l_Buffer = *static_cast<DebugDrawBuffer*>(context);
            for (int l_Index = 0; l_Index < vertexCount; ++l_Index)
            {
                b2Vec2 l_Start = b2TransformPoint(transform, vertices[l_Index]);
                b2Vec2 l_End = b2TransformPoint(transform, vertices[(l_Index + 1) % vertexCount]);
                AppendLine(l_Buffer, l_Start, l_End, color);
            }
        }

        void DrawCircleCallback(b2Vec2 center, float radius, b2HexColor color, void* context)
        {
            DrawCircleSegments(*static_cast<DebugDrawBuffer*>(context), center, radius, color);
        }

        void DrawSolidCircleCallback(b2Transform transform, float radius, b2HexColor color, void* context)
        {
            DebugDrawBuffer& l_Buffer = *static_cast<DebugDrawBuffer*>(context);
            DrawCircleSegments(l_Buffer, transform.p, radius, color);

            // Radius line makes the rotation visible.
            b2Vec2 l_Edge = { transform.p.x + radius * transform.q.c, transform.p.y + radius * transform.q.s };
            AppendLine(l_Buffer, transform.p, l_Edge, color);
        }

        void DrawSolidCapsuleCallback(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* context)
        {
            DebugDrawBuffer& l_Buffer = *static_cast<DebugDrawBuffer*>(context);
            DrawCircleSegments(l_Buffer, p1, radius, color);
            DrawCircleSegments(l_Buffer, p2, radius, color);
            AppendLine(l_Buffer, p1, p2, color);
        }

        void DrawSegmentCallback(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context)
        {
            AppendLine(*static_cast<DebugDrawBuffer*>(context), p1, p2, color);
        }

        void DrawTransformCallback(b2Transform, void*)
        {

        }

        void DrawPointCallback(b2Vec2 p, float size, b2HexColor color, void* context)
        {
            DebugDrawBuffer& l_Buffer = *static_cast<DebugDrawBuffer*>(context);
            float l_Half = size * 0.5f;
            AppendLine(l_Buffer, { p.x - l_Half, p.y }, { p.x + l_Half, p.y }, color);
            AppendLine(l_Buffer, { p.x, p.y - l_Half }, { p.x, p.y + l_Half }, color);
        }

        void DrawStringCallback(b2Vec2, const char*, b2HexColor, void*)
        {

        }
    }

    struct Box2DBackend::Implementation
    {
        struct BodyRecord
        {
            b2BodyId Id = b2_nullBodyId;
            uint32_t Layer = 0;
            std::vector<uint64_t> Shapes;
        };

        b2WorldId World = b2_nullWorldId;
        PhysicsSettings Settings;

        std::unordered_map<uint64_t, BodyRecord> Bodies;
        std::unordered_map<uint64_t, b2ShapeId> Shapes;
        uint64_t NextBodyHandle = 1;
        uint64_t NextShapeHandle = 1;
    };

    Box2DBackend::Box2DBackend() : m_Implementation(std::make_unique<Implementation>())
    {

    }

    Box2DBackend::~Box2DBackend()
    {
        Shutdown();
    }

    bool Box2DBackend::Initialize(const PhysicsSettings& settings)
    {
        b2SetAssertFcn(&Box2DAssertCallback);

        m_Implementation->Settings = settings;

        b2WorldDef l_WorldDef = b2DefaultWorldDef();
        l_WorldDef.gravity = { settings.Gravity2D.x, settings.Gravity2D.y };
        l_WorldDef.enableSleep = true;

        m_Implementation->World = b2CreateWorld(&l_WorldDef);
        if (!b2World_IsValid(m_Implementation->World))
        {
            TR_CORE_ERROR("Failed to create Box2D world");

            return false;
        }

        return true;
    }

    void Box2DBackend::Shutdown()
    {
        if (m_Implementation == nullptr)
        {
            return;
        }

        if (b2World_IsValid(m_Implementation->World))
        {
            b2DestroyWorld(m_Implementation->World);
            m_Implementation->World = b2_nullWorldId;
        }

        m_Implementation->Bodies.clear();
        m_Implementation->Shapes.clear();
    }

    BodyHandle Box2DBackend::CreateBody(const BodyDescription2D& description)
    {
        if (!b2World_IsValid(m_Implementation->World))
        {
            return BodyHandle::Invalid;
        }

        b2BodyDef l_Def = b2DefaultBodyDef();
        switch (description.Type)
        {
        case BodyType::Static: l_Def.type = b2_staticBody; break;
        case BodyType::Kinematic: l_Def.type = b2_kinematicBody; break;
        case BodyType::Dynamic: l_Def.type = b2_dynamicBody; break;
        }

        l_Def.position = { description.Position.x, description.Position.y };
        l_Def.rotation = b2MakeRot(description.Rotation);
        l_Def.linearDamping = description.LinearDamping;
        l_Def.angularDamping = description.AngularDamping;
        l_Def.gravityScale = description.GravityScale;
        l_Def.fixedRotation = description.FixedRotation;
        l_Def.sleepThreshold = m_Implementation->Settings.SleepLinearVelocity;
        l_Def.userData = reinterpret_cast<void*>(static_cast<uintptr_t>(description.UserData));

        b2BodyId l_Body = b2CreateBody(m_Implementation->World, &l_Def);
        if (!b2Body_IsValid(l_Body))
        {
            return BodyHandle::Invalid;
        }

        uint64_t l_Handle = m_Implementation->NextBodyHandle++;
        Implementation::BodyRecord l_Record;
        l_Record.Id = l_Body;
        l_Record.Layer = description.Layer & 31;
        m_Implementation->Bodies.emplace(l_Handle, std::move(l_Record));

        return static_cast<BodyHandle>(l_Handle);
    }

    void Box2DBackend::DestroyBody(BodyHandle body)
    {
        auto l_Found = m_Implementation->Bodies.find(static_cast<uint64_t>(body));
        if (l_Found == m_Implementation->Bodies.end())
        {
            return;
        }

        for (uint64_t it_Shape : l_Found->second.Shapes)
        {
            m_Implementation->Shapes.erase(it_Shape);
        }

        if (b2Body_IsValid(l_Found->second.Id))
        {
            b2DestroyBody(l_Found->second.Id);
        }

        m_Implementation->Bodies.erase(l_Found);
    }

    ShapeHandle Box2DBackend::AddShape(BodyHandle body, const ShapeDescription2D& description)
    {
        auto l_Found = m_Implementation->Bodies.find(static_cast<uint64_t>(body));
        if (l_Found == m_Implementation->Bodies.end() || !b2Body_IsValid(l_Found->second.Id))
        {
            return ShapeHandle::Invalid;
        }

        b2ShapeDef l_ShapeDef = b2DefaultShapeDef();
        l_ShapeDef.density = description.Material.Density;
        l_ShapeDef.material.friction = description.Material.Friction;
        l_ShapeDef.material.restitution = description.Material.Restitution;
        l_ShapeDef.isSensor = description.IsTrigger;
        l_ShapeDef.enableSensorEvents = true;
        l_ShapeDef.enableContactEvents = !description.IsTrigger;
        l_ShapeDef.filter.categoryBits = 1ull << l_Found->second.Layer;
        l_ShapeDef.filter.maskBits = m_Implementation->Settings.LayerCollisionMatrix[l_Found->second.Layer];

        b2ShapeId l_Shape;
        if (description.Type == ShapeType2D::Circle)
        {
            b2Circle l_Circle;
            l_Circle.center = { description.Offset.x, description.Offset.y };
            l_Circle.radius = description.Radius;
            l_Shape = b2CreateCircleShape(l_Found->second.Id, &l_ShapeDef, &l_Circle);
        }
        else
        {
            b2Polygon l_Box = b2MakeOffsetBox(description.HalfExtents.x, description.HalfExtents.y, { description.Offset.x, description.Offset.y }, b2Rot_identity);
            l_Shape = b2CreatePolygonShape(l_Found->second.Id, &l_ShapeDef, &l_Box);
        }

        if (!b2Shape_IsValid(l_Shape))
        {
            return ShapeHandle::Invalid;
        }

        uint64_t l_Handle = m_Implementation->NextShapeHandle++;
        m_Implementation->Shapes.emplace(l_Handle, l_Shape);
        l_Found->second.Shapes.push_back(l_Handle);

        return static_cast<ShapeHandle>(l_Handle);
    }

    void Box2DBackend::SetBodyTransform(BodyHandle body, const glm::vec2& position, float rotation)
    {
        auto l_Found = m_Implementation->Bodies.find(static_cast<uint64_t>(body));
        if (l_Found != m_Implementation->Bodies.end() && b2Body_IsValid(l_Found->second.Id))
        {
            b2Body_SetTransform(l_Found->second.Id, { position.x, position.y }, b2MakeRot(rotation));
        }
    }

    bool Box2DBackend::GetBodyTransform(BodyHandle body, glm::vec2& outPosition, float& outRotation) const
    {
        auto l_Found = m_Implementation->Bodies.find(static_cast<uint64_t>(body));
        if (l_Found == m_Implementation->Bodies.end() || !b2Body_IsValid(l_Found->second.Id))
        {
            return false;
        }

        b2Vec2 l_Position = b2Body_GetPosition(l_Found->second.Id);
        outPosition = glm::vec2(l_Position.x, l_Position.y);
        outRotation = b2Rot_GetAngle(b2Body_GetRotation(l_Found->second.Id));

        return true;
    }

    bool Box2DBackend::IsBodyAwake(BodyHandle body) const
    {
        auto l_Found = m_Implementation->Bodies.find(static_cast<uint64_t>(body));

        return l_Found != m_Implementation->Bodies.end() && b2Body_IsValid(l_Found->second.Id) && b2Body_IsAwake(l_Found->second.Id);
    }

    void Box2DBackend::SetLinearVelocity(BodyHandle body, const glm::vec2& velocity)
    {
        auto l_Found = m_Implementation->Bodies.find(static_cast<uint64_t>(body));
        if (l_Found != m_Implementation->Bodies.end() && b2Body_IsValid(l_Found->second.Id))
        {
            b2Body_SetLinearVelocity(l_Found->second.Id, { velocity.x, velocity.y });
        }
    }

    void Box2DBackend::ApplyForce(BodyHandle body, const glm::vec2& force)
    {
        auto l_Found = m_Implementation->Bodies.find(static_cast<uint64_t>(body));
        if (l_Found != m_Implementation->Bodies.end() && b2Body_IsValid(l_Found->second.Id))
        {
            b2Body_ApplyForceToCenter(l_Found->second.Id, { force.x, force.y }, true);
        }
    }

    void Box2DBackend::ApplyImpulse(BodyHandle body, const glm::vec2& impulse)
    {
        auto l_Found = m_Implementation->Bodies.find(static_cast<uint64_t>(body));
        if (l_Found != m_Implementation->Bodies.end() && b2Body_IsValid(l_Found->second.Id))
        {
            b2Body_ApplyLinearImpulseToCenter(l_Found->second.Id, { impulse.x, impulse.y }, true);
        }
    }

    void Box2DBackend::Step(float fixedDelta)
    {
        if (b2World_IsValid(m_Implementation->World))
        {
            b2World_Step(m_Implementation->World, fixedDelta, static_cast<int>(m_Implementation->Settings.SolverSubSteps));
        }
    }

    bool Box2DBackend::Raycast(const glm::vec2& origin, const glm::vec2& direction, float maxDistance, uint32_t layerMask, RaycastHit2D& outHit) const
    {
        if (!b2World_IsValid(m_Implementation->World) || maxDistance <= 0.0f)
        {
            return false;
        }

        float l_Length = glm::length(direction);
        if (l_Length <= 0.0f)
        {
            return false;
        }

        glm::vec2 l_Direction = direction / l_Length;

        b2QueryFilter l_Filter = b2DefaultQueryFilter();
        l_Filter.categoryBits = ~0ull;
        l_Filter.maskBits = layerMask;

        b2Vec2 l_Translation = { l_Direction.x * maxDistance, l_Direction.y * maxDistance };
        b2RayResult l_Result = b2World_CastRayClosest(m_Implementation->World, { origin.x, origin.y }, l_Translation, l_Filter);
        if (!l_Result.hit || !b2Shape_IsValid(l_Result.shapeId))
        {
            return false;
        }

        outHit.Entity = UUID(ReadBodyUUID(b2Shape_GetBody(l_Result.shapeId)));
        outHit.Point = glm::vec2(l_Result.point.x, l_Result.point.y);
        outHit.Normal = glm::vec2(l_Result.normal.x, l_Result.normal.y);
        outHit.Distance = l_Result.fraction * maxDistance;

        return true;
    }

    void Box2DBackend::DrainEvents(PhysicsEventQueue& outEvents)
    {
        if (!b2World_IsValid(m_Implementation->World))
        {
            return;
        }

        b2ContactEvents l_Contacts = b2World_GetContactEvents(m_Implementation->World);
        for (int l_Index = 0; l_Index < l_Contacts.beginCount; ++l_Index)
        {
            const b2ContactBeginTouchEvent& l_Event = l_Contacts.beginEvents[l_Index];
            if (!b2Shape_IsValid(l_Event.shapeIdA) || !b2Shape_IsValid(l_Event.shapeIdB))
            {
                continue;
            }

            ContactEvent l_Contact;
            l_Contact.A = UUID(ReadBodyUUID(b2Shape_GetBody(l_Event.shapeIdA)));
            l_Contact.B = UUID(ReadBodyUUID(b2Shape_GetBody(l_Event.shapeIdB)));
            l_Contact.Phase = ContactPhase::Begin;
            l_Contact.Normal = glm::vec3(l_Event.manifold.normal.x, l_Event.manifold.normal.y, 0.0f);

            if (l_Event.manifold.pointCount > 0)
            {
                l_Contact.Point = glm::vec3(l_Event.manifold.points[0].point.x, l_Event.manifold.points[0].point.y, 0.0f);
                l_Contact.Impulse = l_Event.manifold.points[0].normalImpulse;
            }

            outEvents.Contacts.push_back(l_Contact);
        }

        for (int l_Index = 0; l_Index < l_Contacts.endCount; ++l_Index)
        {
            const b2ContactEndTouchEvent& l_Event = l_Contacts.endEvents[l_Index];
            if (!b2Shape_IsValid(l_Event.shapeIdA) || !b2Shape_IsValid(l_Event.shapeIdB))
            {
                continue;
            }

            ContactEvent l_Contact;
            l_Contact.A = UUID(ReadBodyUUID(b2Shape_GetBody(l_Event.shapeIdA)));
            l_Contact.B = UUID(ReadBodyUUID(b2Shape_GetBody(l_Event.shapeIdB)));
            l_Contact.Phase = ContactPhase::End;

            outEvents.Contacts.push_back(l_Contact);
        }

        b2SensorEvents l_Sensors = b2World_GetSensorEvents(m_Implementation->World);
        for (int l_Index = 0; l_Index < l_Sensors.beginCount; ++l_Index)
        {
            const b2SensorBeginTouchEvent& l_Event = l_Sensors.beginEvents[l_Index];
            if (!b2Shape_IsValid(l_Event.sensorShapeId) || !b2Shape_IsValid(l_Event.visitorShapeId))
            {
                continue;
            }

            TriggerEvent l_Trigger;
            l_Trigger.Trigger = UUID(ReadBodyUUID(b2Shape_GetBody(l_Event.sensorShapeId)));
            l_Trigger.Other = UUID(ReadBodyUUID(b2Shape_GetBody(l_Event.visitorShapeId)));
            l_Trigger.Phase = ContactPhase::Begin;

            outEvents.Triggers.push_back(l_Trigger);
        }

        for (int l_Index = 0; l_Index < l_Sensors.endCount; ++l_Index)
        {
            const b2SensorEndTouchEvent& l_Event = l_Sensors.endEvents[l_Index];
            if (!b2Shape_IsValid(l_Event.sensorShapeId) || !b2Shape_IsValid(l_Event.visitorShapeId))
            {
                continue;
            }

            TriggerEvent l_Trigger;
            l_Trigger.Trigger = UUID(ReadBodyUUID(b2Shape_GetBody(l_Event.sensorShapeId)));
            l_Trigger.Other = UUID(ReadBodyUUID(b2Shape_GetBody(l_Event.visitorShapeId)));
            l_Trigger.Phase = ContactPhase::End;

            outEvents.Triggers.push_back(l_Trigger);
        }
    }

    void Box2DBackend::GetDebugLines(DebugDrawBuffer& outBuffer) const
    {
        if (!b2World_IsValid(m_Implementation->World))
        {
            return;
        }

        b2DebugDraw l_Draw = b2DefaultDebugDraw();
        l_Draw.DrawPolygonFcn = &DrawPolygonCallback;
        l_Draw.DrawSolidPolygonFcn = &DrawSolidPolygonCallback;
        l_Draw.DrawCircleFcn = &DrawCircleCallback;
        l_Draw.DrawSolidCircleFcn = &DrawSolidCircleCallback;
        l_Draw.DrawSolidCapsuleFcn = &DrawSolidCapsuleCallback;
        l_Draw.DrawSegmentFcn = &DrawSegmentCallback;
        l_Draw.DrawTransformFcn = &DrawTransformCallback;
        l_Draw.DrawPointFcn = &DrawPointCallback;
        l_Draw.DrawStringFcn = &DrawStringCallback;
        l_Draw.drawShapes = true;
        l_Draw.context = &outBuffer;

        b2World_Draw(m_Implementation->World, &l_Draw);
    }
}