#include "Trinity/Utilities/Time.h"
#include "Trinity/Utilities/Log.h"

#include <chrono>

namespace Trinity
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        Clock::time_point s_StartTime;
        Clock::time_point s_LastFrameTime;
    }

    namespace Utilities
    {

        bool Time::s_Initialized = false;
        float Time::s_DeltaTime = 0.0f;

        void Time::Initialize()
        {
            s_StartTime = Clock::now();
            s_LastFrameTime = s_StartTime;
            s_DeltaTime = 0.0f;
            s_Initialized = true;

            TR_CORE_INFO("TIME INITIALIZED");
        }

        void Time::Update()
        {
            if (!s_Initialized)
            {
                Initialize();
            }

            const auto l_Now = Clock::now();
            const std::chrono::duration<float> l_DeltaTime = l_Now - s_LastFrameTime;

            s_DeltaTime = l_DeltaTime.count();
            s_LastFrameTime = l_Now;
        }

        double Time::Now()
        {
            if (!s_Initialized)
            {
                Initialize();
            }

            const auto l_Now = Clock::now();
            const std::chrono::duration<double> l_Elapsed = l_Now - s_StartTime;

            return l_Elapsed.count();
        }

        float Time::DeltaTime()
        {
            return s_DeltaTime;
        }
    }
}