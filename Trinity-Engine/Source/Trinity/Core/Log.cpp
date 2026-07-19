#include <Trinity/Core/Log.h>

#include <deque>
#include <mutex>
#include <utility>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Trinity
{
    namespace
    {
        constexpr size_t k_MaxMessages = 5000;

        std::deque<LogMessage> g_Messages;
        std::mutex g_MessagesMutex;
    }

    std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
    std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

    void Log::Initialize()
    {
        auto a_ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        a_ConsoleSink->set_pattern("%^[%T] [%n] [%s:%#] [%!] %v%$");

        s_CoreLogger = std::make_shared<spdlog::logger>("TRINITY", a_ConsoleSink);
        s_CoreLogger->set_level(spdlog::level::trace);
        s_CoreLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(s_CoreLogger);

        s_ClientLogger = std::make_shared<spdlog::logger>("APP", a_ConsoleSink);
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
            a_FileSink->set_pattern("[%d-%m-%Y] %^[%T] [%n] [%l] [%s:%#] [%!] %v");

            s_CoreLogger->sinks().push_back(a_FileSink);
            s_ClientLogger->sinks().push_back(a_FileSink);

            ("Log: file sink at {}", path.string());
        }
        catch (const spdlog::spdlog_ex& l_Exception)
        {
            ("Log: failed to create file sink at {}: {}", path.string(), l_Exception.what());
        }
    }

    void Log::Shutdown()
    {
        s_ClientLogger.reset();
        s_CoreLogger.reset();
        spdlog::drop_all();

        std::lock_guard<std::mutex> l_Lock(g_MessagesMutex);
        g_Messages.clear();
    }

    void Log::WriteConsole(LogLevel level, const std::string& text)
    {
        LogMessage l_Entry;
        l_Entry.Level = level;
        l_Entry.Text = text;

        std::lock_guard<std::mutex> l_Lock(g_MessagesMutex);
        g_Messages.push_back(std::move(l_Entry));
        if (g_Messages.size() > k_MaxMessages)
        {
            g_Messages.pop_front();
        }
    }

    void Log::VisitMessages(const std::function<void(const LogMessage&)>& visitor)
    {
        std::lock_guard<std::mutex> l_Lock(g_MessagesMutex);
        for (const LogMessage& it_Message : g_Messages)
        {
            visitor(it_Message);
        }
    }

    void Log::ClearMessages()
    {
        std::lock_guard<std::mutex> l_Lock(g_MessagesMutex);
        g_Messages.clear();
    }
}