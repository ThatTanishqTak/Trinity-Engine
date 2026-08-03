#pragma once

#include <cstdint>
#include <cmath>

namespace Trinity
{
    class SimulationClock
    {
    public:
        static constexpr float DefaultFixedDelta = 1.0f / 60.0f;
        static constexpr uint32_t DefaultMaxSubSteps = 8;
        static constexpr float MaxFrameDelta = 0.25f;

    public:
        // Feeds one frame's measured delta into the accumulator and returns how many fixed steps to run this frame. The frame delta is clamped (spiral-of-death guard) and the step count is bounded by MaxSubSteps; any simulated time
        // discarded by either guard is reported through ConsumeDroppedTime
        uint32_t BeginFrame(float frameDelta)
        {
            if (frameDelta > MaxFrameDelta)
            {
                m_DroppedTime += frameDelta - MaxFrameDelta;
                frameDelta = MaxFrameDelta;
            }

            m_Accumulator += frameDelta;

            uint32_t l_Steps = static_cast<uint32_t>(m_Accumulator / m_FixedDelta);
            if (l_Steps > m_MaxSubSteps)
            {
                l_Steps = m_MaxSubSteps;

                float l_Bounded = static_cast<float>(l_Steps) * m_FixedDelta + std::fmod(m_Accumulator, m_FixedDelta);
                m_DroppedTime += m_Accumulator - l_Bounded;
                m_Accumulator = l_Bounded;
            }

            m_Accumulator -= static_cast<float>(l_Steps) * m_FixedDelta;

            // Alpha is only valid after the drain above; computing it earlier is the classic interpolation bug.
            m_Alpha = m_Accumulator / m_FixedDelta;

            return l_Steps;
        }

        float ConsumeDroppedTime()
        {
            float l_Dropped = m_DroppedTime;
            m_DroppedTime = 0.0f;

            return l_Dropped;
        }

        void Reset()
        {
            m_Accumulator = 0.0f;
            m_Alpha = 0.0f;
            m_DroppedTime = 0.0f;
        }

        void SetFixedDelta(float fixedDelta)
        {
            if (fixedDelta > 0.0f)
            {
                m_FixedDelta = fixedDelta;
            }
        }

        void SetMaxSubSteps(uint32_t maxSubSteps)
        {
            if (maxSubSteps > 0)
            {
                m_MaxSubSteps = maxSubSteps;
            }
        }

        float GetFixedDelta() const { return m_FixedDelta; }
        uint32_t GetMaxSubSteps() const { return m_MaxSubSteps; }
        float GetAccumulator() const { return m_Accumulator; }
        float GetAlpha() const { return m_Alpha; }

    private:
        float m_FixedDelta = DefaultFixedDelta;
        uint32_t m_MaxSubSteps = DefaultMaxSubSteps;

        float m_Accumulator = 0.0f;
        float m_Alpha = 0.0f;
        float m_DroppedTime = 0.0f;
    };
}