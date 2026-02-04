#include "Trinity/Utilities/Utilities.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <cstdlib>
#include <chrono>
#include <fstream>
#include <vector>

namespace Trinity
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
            logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("TrinityEngine.log", true));

            logSinks[0]->set_pattern("%^[%T] %n: %v%$");
            logSinks[1]->set_pattern("[%T] [%l] %n: %v");

            s_CoreLogger = std::make_shared<spdlog::logger>("TRINITY-ENGINE", begin(logSinks), end(logSinks));
            spdlog::register_logger(s_CoreLogger);
            s_CoreLogger->set_level(spdlog::level::trace);
            s_CoreLogger->flush_on(spdlog::level::trace);

            s_ClientLogger = std::make_shared<spdlog::logger>("TRINITY-FORGE", begin(logSinks), end(logSinks));
            spdlog::register_logger(s_ClientLogger);
            s_ClientLogger->set_level(spdlog::level::trace);
            s_ClientLogger->flush_on(spdlog::level::trace);

            TR_CORE_INFO("LOGGING INITIALIZED");
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

        // -------------------- Vulkan --------------------

        void VulkanUtilities::VKCheck(VkResult result, const char* what)
        {
            if (result != VK_SUCCESS)
            {
                TR_CORE_CRITICAL("Vulkan failure: {} (VkResult = {})", what, (int)result);

                std::abort();
            }
        }

        // -------------------- File Management --------------------
        
        std::vector<char> FileManagement::LoadFromFile(const std::string& path)
        {
            std::ifstream l_File(path, std::ios::ate | std::ios::binary);
            if (!l_File.is_open())
            {
                TR_CORE_CRITICAL("Failed to open file: {}", path.c_str());

                std::abort();
            }

            const size_t l_FileSize = (size_t)l_File.tellg();
            std::vector<char> l_Buffer(l_FileSize);

            l_File.seekg(0);
            l_File.read(l_Buffer.data(), (std::streamsize)l_FileSize);
            l_File.close();

            return l_Buffer;
        }

        void FileManagement::SaveToFile(const std::string& path, const std::vector<char>& data)
        {
            const std::filesystem::path l_Path(path);
            const std::filesystem::path l_ParentPath = l_Path.parent_path();
            if (!l_ParentPath.empty())
            {
                std::error_code l_Error;
                std::filesystem::create_directories(l_ParentPath, l_Error);
            }

            std::ofstream l_File(path, std::ios::binary | std::ios::trunc);
            if (!l_File.is_open())
            {
                TR_CORE_ERROR("Failed to open file for writing: {}", path.c_str());

                return;
            }

            if (!data.empty())
            {
                l_File.write(data.data(), static_cast<std::streamsize>(data.size()));
            }
        }
    }
}