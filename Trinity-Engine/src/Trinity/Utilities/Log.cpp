#include "Trinity/Utilities/Log.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <vector>

namespace Trinity
{
    namespace CoreUtilities
    {
        std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
        std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

        void Log::Initialize()
        {
            std::vector<spdlog::sink_ptr> l_LogSinks;

            l_LogSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
            l_LogSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Engine_Log.txt", true));

            // Console
            l_LogSinks[0]->set_pattern("%^[%T.%e] [%n] [%s:%#] %v%$");
            // File
            l_LogSinks[1]->set_pattern("[%d-%m-%Y %T.%e] [%n] [%-l] [pid:%P] [thread:%t] [%s:%#] [%!] %v");

            s_CoreLogger = std::make_shared<spdlog::logger>("TRINITY-ENGINE", begin(l_LogSinks), end(l_LogSinks));

            spdlog::register_logger(s_CoreLogger);
            s_CoreLogger->set_level(spdlog::level::trace);
            s_CoreLogger->flush_on(spdlog::level::trace);
            s_ClientLogger = std::make_shared<spdlog::logger>("TRINITY-FORGE", begin(l_LogSinks), end(l_LogSinks));

            spdlog::register_logger(s_ClientLogger);
            s_ClientLogger->set_level(spdlog::level::trace);
            s_ClientLogger->flush_on(spdlog::level::trace);
        }
    }
}