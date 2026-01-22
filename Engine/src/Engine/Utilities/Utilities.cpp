#include "Engine/Utilities/Utilities.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <chrono>

namespace Engine
{
    namespace Utilities
    {
        // -------------------- Log --------------------

        std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
        std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

        void Log::Initialize()
        {
            std::vector<spdlog::sink_ptr> logSinks;
            logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
            logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Engine.log", true));

            logSinks[0]->set_pattern("%^[%T] %n: %v%$");
            logSinks[1]->set_pattern("[%T] [%l] %n: %v");

            s_CoreLogger = std::make_shared<spdlog::logger>("ENGINE", begin(logSinks), end(logSinks));
            spdlog::register_logger(s_CoreLogger);
            s_CoreLogger->set_level(spdlog::level::trace);
            s_CoreLogger->flush_on(spdlog::level::trace);

            s_ClientLogger = std::make_shared<spdlog::logger>("CLIENT", begin(logSinks), end(logSinks));
            spdlog::register_logger(s_ClientLogger);
            s_ClientLogger->set_level(spdlog::level::trace);
            s_ClientLogger->flush_on(spdlog::level::trace);
        }

        // -------------------- Time --------------------

        namespace
        {
            using Clock = std::chrono::steady_clock;

            Clock::time_point s_StartTime;
            Clock::time_point s_LastFrameTime;
        }

        bool Time::s_Initialized = false;
        float Time::s_DeltaTime = 0.0f;

        void Time::Initialize()
        {
            s_StartTime = Clock::now();
            s_LastFrameTime = s_StartTime;
            s_DeltaTime = 0.0f;
            s_Initialized = true;
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