#pragma once

#include <array>
#include <cstdint>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <Trinity/Physics/PhysicsTypes.h>

namespace Trinity
{
    struct PhysicsSettings
    {
        static constexpr std::array<uint32_t, 32> DefaultLayerMatrix()
        {
            std::array<uint32_t, 32> l_Matrix{};
            for (uint32_t& it_Row : l_Matrix)
            {
                it_Row = 0xFFFFFFFF;
            }

            return l_Matrix;
        }

        PhysicsBackend2D Backend2D = PhysicsBackend2D::Box2D;
        PhysicsBackend3D Backend3D = PhysicsBackend3D::PhysX;

        glm::vec2 Gravity2D{ 0.0f, -9.81f };
        glm::vec3 Gravity3D{ 0.0f, -9.81f, 0.0f };

        // Reference only; the authoritative step interval is the SimulationClock's.
        float FixedDelta = 1.0f / 60.0f;

        uint32_t SolverSubSteps = 4;          // Box2D sub-step count / PhysX position iterations
        uint32_t SolverVelocityIterations = 1;

        float SleepLinearVelocity = 0.05f;    // m/s; below this a body may start sleeping
        float SleepAngularVelocity = 0.05f;   // rad/s
        float SleepTime = 0.5f;               // seconds spent below both thresholds

        // Row i is a bit mask of the layers that layer i collides with.
        std::array<uint32_t, 32> LayerCollisionMatrix = DefaultLayerMatrix();

        bool LayersCollide(uint32_t layerA, uint32_t layerB) const
        {
            return (LayerCollisionMatrix[layerA & 31] & (1u << (layerB & 31))) != 0;
        }
    };
}