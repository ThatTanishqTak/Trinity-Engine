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

        static void WriteConsole(LogLevel level, const std::string& text);

        static void VisitMessages(const std::function<void(const LogMessage&)>& visitor);
        static void ClearMessages();

        static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };
}

#define TR_INTERNAL_LOG(logger, level, ...) (logger)->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, level, __VA_ARGS__)

#define TR_CORE_TRACE(...) TR_INTERNAL_LOG(::Trinity::Log::GetCoreLogger(), spdlog::level::trace, __VA_ARGS__)
#define TR_CORE_INFO(...) TR_INTERNAL_LOG(::Trinity::Log::GetCoreLogger(), spdlog::level::info, __VA_ARGS__)
#define TR_CORE_WARN(...) TR_INTERNAL_LOG(::Trinity::Log::GetCoreLogger(), spdlog::level::warn, __VA_ARGS__)
#define TR_CORE_ERROR(...) TR_INTERNAL_LOG(::Trinity::Log::GetCoreLogger(), spdlog::level::err, __VA_ARGS__)
#define TR_CORE_CRITICAL(...) TR_INTERNAL_LOG(::Trinity::Log::GetCoreLogger(), spdlog::level::critical, __VA_ARGS__)

#define TR_TRACE(...) TR_INTERNAL_LOG(::Trinity::Log::GetClientLogger(), spdlog::level::trace, __VA_ARGS__)
#define TR_INFO(...) TR_INTERNAL_LOG(::Trinity::Log::GetClientLogger(), spdlog::level::info, __VA_ARGS__)
#define TR_WARN(...) TR_INTERNAL_LOG(::Trinity::Log::GetClientLogger(), spdlog::level::warn, __VA_ARGS__)
#define TR_ERROR(...) TR_INTERNAL_LOG(::Trinity::Log::GetClientLogger(), spdlog::level::err, __VA_ARGS__)
#define TR_CRITICAL(...) TR_INTERNAL_LOG(::Trinity::Log::GetClientLogger(), spdlog::level::critical, __VA_ARGS__)