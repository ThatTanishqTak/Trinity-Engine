#include <Trinity/Physics/Backends/Box2D/Box2DBackend.h>
#include <Trinity/Core/Log.h>

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace Trinity;

namespace
{
    constexpr float k_Delta = 1.0f / 60.0f;

    BodyHandle MakeGround(Box2DBackend& backend, uint64_t uuid)
    {
        BodyDescription2D l_Description;
        l_Description.Type = BodyType::Static;
        l_Description.Position = { 0.0f, -0.5f };
        l_Description.UserData = uuid;
        BodyHandle l_Ground = backend.CreateBody(l_Description);

        ShapeDescription2D l_Shape;
        l_Shape.HalfExtents = { 50.0f, 0.5f };
        backend.AddShape(l_Ground, l_Shape);

        return l_Ground;
    }

    BodyHandle MakeUnitBox(Box2DBackend& backend, float x, float y, uint64_t uuid)
    {
        BodyDescription2D l_Description;
        l_Description.Type = BodyType::Dynamic;
        l_Description.Position = { x, y };
        l_Description.UserData = uuid;
        BodyHandle l_Body = backend.CreateBody(l_Description);

        ShapeDescription2D l_Shape;
        l_Shape.HalfExtents = { 0.5f, 0.5f };
        backend.AddShape(l_Body, l_Shape);

        return l_Body;
    }
}

// 10 m drop lands at ~1.43 s, rests at y = 0.5, and answers a raycast.
static void TestDrop()
{
    Box2DBackend l_Backend;
    PhysicsSettings l_Settings;
    bool l_Ok = l_Backend.Initialize(l_Settings);
    assert(l_Ok);

    MakeGround(l_Backend, 1);
    BodyHandle l_Box = MakeUnitBox(l_Backend, 0.0f, 10.5f, 2);

    PhysicsEventQueue l_Events;
    int l_ContactStep = -1;
    for (int l_Step = 1; l_Step <= 60 * 5; ++l_Step)
    {
        l_Backend.Step(k_Delta);
        l_Events.Clear();
        l_Backend.DrainEvents(l_Events);
        if (l_ContactStep < 0 && !l_Events.Contacts.empty())
        {
            l_ContactStep = l_Step;
        }
    }

    assert(l_ContactStep > 0);
    float l_ContactTime = static_cast<float>(l_ContactStep) * k_Delta;
    std::printf("drop: first contact at %.3f s (analytic 1.428 s)\n", l_ContactTime);
    assert(std::fabs(l_ContactTime - 1.428f) < 0.08f);

    glm::vec2 l_Position;
    float l_Rotation = 0.0f;
    l_Ok = l_Backend.GetBodyTransform(l_Box, l_Position, l_Rotation);
    assert(l_Ok);
    std::printf("drop: rest position (%.4f, %.4f), rotation %.4f\n", l_Position.x, l_Position.y, l_Rotation);
    assert(std::fabs(l_Position.y - 0.5f) < 0.02f);
    assert(std::fabs(l_Position.x) < 0.01f);

    RaycastHit2D l_Hit;
    l_Ok = l_Backend.Raycast({ 0.0f, 5.0f }, { 0.0f, -1.0f }, 20.0f, 0xFFFFFFFF, l_Hit);
    assert(l_Ok);
    std::printf("raycast: entity %llu at (%.3f, %.3f), distance %.3f\n",
        static_cast<unsigned long long>(static_cast<uint64_t>(l_Hit.Entity)), l_Hit.Point.x, l_Hit.Point.y, l_Hit.Distance);
    assert(static_cast<uint64_t>(l_Hit.Entity) == 2);
    assert(std::fabs(l_Hit.Distance - 4.0f) < 0.05f);

    DebugDrawBuffer l_DebugBuffer;
    l_Backend.GetDebugLines(l_DebugBuffer);
    std::printf("debug draw: %zu lines\n", l_DebugBuffer.Lines.size());
    assert(!l_DebugBuffer.Lines.empty());
}

// A stack of 20 boxes remains stable for 60 simulated seconds.
static void TestStack()
{
    Box2DBackend l_Backend;
    PhysicsSettings l_Settings;
    bool l_Ok = l_Backend.Initialize(l_Settings);
    assert(l_Ok);

    MakeGround(l_Backend, 1);

    BodyHandle l_Top = BodyHandle::Invalid;
    for (int l_Index = 0; l_Index < 20; ++l_Index)
    {
        l_Top = MakeUnitBox(l_Backend, 0.0f, 0.5f + static_cast<float>(l_Index) * 1.0f, 100 + l_Index);
    }

    for (int l_Step = 0; l_Step < 60 * 60; ++l_Step)
    {
        l_Backend.Step(k_Delta);
    }

    glm::vec2 l_Position;
    float l_Rotation = 0.0f;
    l_Ok = l_Backend.GetBodyTransform(l_Top, l_Position, l_Rotation);
    assert(l_Ok);
    std::printf("stack: top box after 60 s at (%.4f, %.4f)\n", l_Position.x, l_Position.y);
    assert(std::fabs(l_Position.y - 19.5f) < 0.3f);
    assert(std::fabs(l_Position.x) < 0.2f);
}

// Triggers fire begin and end exactly once for a body falling through a sensor.
static void TestTrigger()
{
    Box2DBackend l_Backend;
    PhysicsSettings l_Settings;
    bool l_Ok = l_Backend.Initialize(l_Settings);
    assert(l_Ok);

    MakeGround(l_Backend, 1);

    BodyDescription2D l_SensorBody;
    l_SensorBody.Type = BodyType::Static;
    l_SensorBody.Position = { 0.0f, 5.0f };
    l_SensorBody.UserData = 42;
    BodyHandle l_Sensor = l_Backend.CreateBody(l_SensorBody);

    ShapeDescription2D l_SensorShape;
    l_SensorShape.HalfExtents = { 2.0f, 0.5f };
    l_SensorShape.IsTrigger = true;
    l_Backend.AddShape(l_Sensor, l_SensorShape);

    MakeUnitBox(l_Backend, 0.0f, 10.5f, 2);

    int l_BeginCount = 0;
    int l_EndCount = 0;
    PhysicsEventQueue l_Events;
    for (int l_Step = 0; l_Step < 60 * 5; ++l_Step)
    {
        l_Backend.Step(k_Delta);
        l_Events.Clear();
        l_Backend.DrainEvents(l_Events);

        for (const TriggerEvent& it_Event : l_Events.Triggers)
        {
            assert(static_cast<uint64_t>(it_Event.Trigger) == 42);
            assert(static_cast<uint64_t>(it_Event.Other) == 2);
            if (it_Event.Phase == ContactPhase::Begin)
            {
                ++l_BeginCount;
            }
            else
            {
                ++l_EndCount;
            }
        }
    }

    std::printf("trigger: %d begin, %d end (expected 1 / 1)\n", l_BeginCount, l_EndCount);
    assert(l_BeginCount == 1);
    assert(l_EndCount == 1);
}

// Two layers configured not to collide pass through each other.
static void TestLayerFilter()
{
    Box2DBackend l_Backend;
    PhysicsSettings l_Settings;
    l_Settings.LayerCollisionMatrix[1] &= ~(1u << 0);
    l_Settings.LayerCollisionMatrix[0] &= ~(1u << 1);
    bool l_Ok = l_Backend.Initialize(l_Settings);
    assert(l_Ok);

    MakeGround(l_Backend, 1);

    BodyDescription2D l_Description;
    l_Description.Type = BodyType::Dynamic;
    l_Description.Position = { 0.0f, 5.0f };
    l_Description.Layer = 1;
    l_Description.UserData = 2;
    BodyHandle l_Body = l_Backend.CreateBody(l_Description);

    ShapeDescription2D l_Shape;
    l_Shape.HalfExtents = { 0.5f, 0.5f };
    l_Backend.AddShape(l_Body, l_Shape);

    for (int l_Step = 0; l_Step < 60 * 2; ++l_Step)
    {
        l_Backend.Step(k_Delta);
    }

    glm::vec2 l_Position;
    float l_Rotation = 0.0f;
    l_Ok = l_Backend.GetBodyTransform(l_Body, l_Position, l_Rotation);
    assert(l_Ok);
    std::printf("layers: filtered box fell to y = %.3f\n", l_Position.y);
    assert(l_Position.y < -5.0f);
}

int main()
{
    Log::Initialize();

    TestDrop();
    TestStack();
    TestTrigger();
    TestLayerFilter();

    std::printf("all box2d backend smoke tests passed\n");

    return 0;
}