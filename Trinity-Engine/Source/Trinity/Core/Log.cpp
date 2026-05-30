#include <Trinity/Core/Log.h>

#include <vector>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Trinity
{
    std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
    std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

    void Log::Initialize()
    {
        auto l_ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        l_ConsoleSink->set_pattern("%^[%T] [%n] %v%$");

        s_CoreLogger = std::make_shared<spdlog::logger>("TRINITY", l_ConsoleSink);
        s_CoreLogger->set_level(spdlog::level::trace);
        s_CoreLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(s_CoreLogger);

        s_ClientLogger = std::make_shared<spdlog::logger>("APP", l_ConsoleSink);
        s_ClientLogger->set_level(spdlog::level::trace);
        s_ClientLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(s_ClientLogger);
    }

    void Log::InitializeFileSink(const std::filesystem::path& path)
    {
        if (!s_CoreLogger || !s_ClientLogger)
        {
            return;
        }

        try
        {
            auto a_FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
            a_FileSink->set_pattern("[%Y-%m-%d %T] [%n] [%l] %v");

            s_CoreLogger->sinks().push_back(a_FileSink);
            s_ClientLogger->sinks().push_back(a_FileSink);

            TR_CORE_INFO("Log: file sink at {}", path.string());
        }
        catch (const spdlog::spdlog_ex& l_Exception)
        {
            TR_CORE_ERROR("Log: failed to create file sink at {}: {}", path.string(), l_Exception.what());
        }
    }

    void Log::Shutdown()
    {
        s_ClientLogger.reset();
        s_CoreLogger.reset();
        spdlog::drop_all();
    }
}