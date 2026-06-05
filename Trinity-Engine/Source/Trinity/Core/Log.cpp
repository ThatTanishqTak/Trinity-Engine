#include <Trinity/Core/Log.h>

#include <vector>
#include <deque>
#include <mutex>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/base_sink.h>

namespace Trinity
{
    namespace
    {
        constexpr size_t k_MaxMessages = 1000;

        std::deque<LogMessage> g_Messages;
        std::mutex g_MessagesMutex;

        LogLevel ToLogLevel(spdlog::level::level_enum level)
        {
            switch (level)
            {
                case spdlog::level::trace: return LogLevel::Trace;
                case spdlog::level::debug: return LogLevel::Trace;
                case spdlog::level::info: return LogLevel::Info;
                case spdlog::level::warn: return LogLevel::Warn;
                case spdlog::level::err: return LogLevel::Error;
                case spdlog::level::critical: return LogLevel::Critical;
                default: return LogLevel::Info;
            }
        }

        class EditorRingSink : public spdlog::sinks::base_sink<std::mutex>
        {
        protected:
            void sink_it_(const spdlog::details::log_msg& message) override
            {
                LogMessage l_Entry;
                l_Entry.Level = ToLogLevel(message.level);
                l_Entry.Text.reserve(message.logger_name.size() + message.payload.size() + 3);
                l_Entry.Text.append("[").append(message.logger_name.data(), message.logger_name.size()).append("] ").append(message.payload.data(), message.payload.size());

                std::lock_guard<std::mutex> l_Lock(g_MessagesMutex);
                g_Messages.push_back(std::move(l_Entry));
                if (g_Messages.size() > k_MaxMessages)
                {
                    g_Messages.pop_front();
                }
            }

            void flush_() override
            {

            }
        };
    }

    std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
    std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

    void Log::Initialize()
    {
        auto a_ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        a_ConsoleSink->set_pattern("%^[%T] [%n] %v%$");

        auto a_RingSink = std::make_shared<EditorRingSink>();

        s_CoreLogger = std::make_shared<spdlog::logger>("TRINITY", a_ConsoleSink);
        s_CoreLogger->sinks().push_back(a_RingSink);
        s_CoreLogger->set_level(spdlog::level::trace);
        s_CoreLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(s_CoreLogger);

        s_ClientLogger = std::make_shared<spdlog::logger>("APP", a_ConsoleSink);
        s_ClientLogger->sinks().push_back(a_RingSink);
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

        std::lock_guard<std::mutex> l_Lock(g_MessagesMutex);
        g_Messages.clear();
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