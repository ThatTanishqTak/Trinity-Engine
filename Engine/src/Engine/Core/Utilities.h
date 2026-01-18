#pragma once

// MSVC-only warning suppression
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <memory>

namespace Engine
{
    namespace Utilities
    {
        //---------------------------------------------------------------------------------------------------------//

        class Log
        {
        public:
            static void Initialize();

            static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
            static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

        private:
            static std::shared_ptr<spdlog::logger> s_CoreLogger;
            static std::shared_ptr<spdlog::logger> s_ClientLogger;
        };

        //---------------------------------------------------------------------------------------------------------//

        class Time
        {
        public:
            // Call once during engine startup
            static void Initialize();
            static void Update();

            static double Now();
            static float DeltaTime();

        private:
            static bool s_Initialized;
            static float s_DeltaTime;
        };

        //---------------------------------------------------------------------------------------------------------//
    }
}

// Core log macros
#define TR_CORE_TRACE(...) ::Engine::Utilities::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define TR_CORE_INFO(...)  ::Engine::Utilities::Log::GetCoreLogger()->info(__VA_ARGS__)
#define TR_CORE_WARN(...)  ::Engine::Utilities::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define TR_CORE_ERROR(...) ::Engine::Utilities::Log::GetCoreLogger()->error(__VA_ARGS__)
#define TR_CORE_CRITICAL(...) ::Engine::Utilities::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define TR_TRACE(...) ::Engine::Utilities::Log::GetClientLogger()->trace(__VA_ARGS__)
#define TR_INFO(...)  ::Engine::Utilities::Log::GetClientLogger()->info(__VA_ARGS__)
#define TR_WARN(...)  ::Engine::Utilities::Log::GetClientLogger()->warn(__VA_ARGS__)
#define TR_ERROR(...) ::Engine::Utilities::Log::GetClientLogger()->error(__VA_ARGS__)
#define TR_CRITICAL(...) ::Engine::Utilities::Log::GetClientLogger()->critical(__VA_ARGS__)