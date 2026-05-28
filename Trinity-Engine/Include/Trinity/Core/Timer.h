#pragma once

#include <chrono>

namespace Trinity
{
    class Timer
    {
    public:
        Timer()
        {
            Reset();
        }

        void Reset()
        {
            m_Start = std::chrono::high_resolution_clock::now();
        }

        float Elapsed() const
        {
            auto a_Now = std::chrono::high_resolution_clock::now();
            auto a_Nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(a_Now - m_Start).count();

            return static_cast<float>(a_Nanoseconds) * 0.000000001f;
        }

        float ElapsedMilliseconds() const { return Elapsed() * 1000.0f; }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
    };
}