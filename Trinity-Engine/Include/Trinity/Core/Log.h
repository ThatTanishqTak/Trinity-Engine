#pragma once

#include <memory>
#include <filesystem>
#include <functional>
#include <string>

#include <spdlog/spdlog.h>

namespace Trinity
{
    enum class LogLevel
    {
        Trace,
        Info,
        Warn,
        Error,
        Critical
    };

    struct LogMessage
    {
        LogLevel Level = LogLevel::Info;
        std::string Text;
    };

    class Log
    {
    public:
        static void Initialize();
        static void InitializeFileSink(const std::filesystem::path& path);
        static void Shutdown();

        static void VisitMessages(const std::function<void(const LogMessage&)>& visitor);
        static void ClearMessages();

        static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };
}

#define TR_CORE_TRACE(...) ::Trinity::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define TR_CORE_INFO(...) ::Trinity::Log::GetCoreLogger()->info(__VA_ARGS__)
#define TR_CORE_WARN(...) ::Trinity::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define TR_CORE_ERROR(...) ::Trinity::Log::GetCoreLogger()->error(__VA_ARGS__)
#define TR_CORE_CRITICAL(...) ::Trinity::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define TR_TRACE(...) ::Trinity::Log::GetClientLogger()->trace(__VA_ARGS__)
#define TR_INFO(...) ::Trinity::Log::GetClientLogger()->info(__VA_ARGS__)
#define TR_WARN(...) ::Trinity::Log::GetClientLogger()->warn(__VA_ARGS__)
#define TR_ERROR(...) ::Trinity::Log::GetClientLogger()->error(__VA_ARGS__)
#define TR_CRITICAL(...) ::Trinity::Log::GetClientLogger()->critical(__VA_ARGS__)