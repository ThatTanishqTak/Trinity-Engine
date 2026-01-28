#pragma once

// MSVC-only warning suppression
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <vulkan/vulkan.h>

#include <memory>
#include <string>
#include <vector>

template<glm::length_t L, typename T, glm::qualifier Q>
struct fmt::formatter<glm::vec<L, T, Q>> : formatter<std::string>
{
    template<typename FormatContext>
    auto format(const glm::vec<L, T, Q>& v, FormatContext& ctx) const
    {
        return formatter<std::string>::format(glm::to_string(v), ctx);
    }
};

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
struct fmt::formatter<glm::mat<C, R, T, Q>> : formatter<std::string>
{
    template<typename FormatContext>
    auto format(const glm::mat<C, R, T, Q>& m, FormatContext& ctx) const
    {
        return formatter<std::string>::format(glm::to_string(m), ctx);
    }
};

template<typename T, glm::qualifier Q>
struct fmt::formatter<glm::qua<T, Q>> : formatter<std::string>
{
    template<typename FormatContext>
    auto format(const glm::qua<T, Q>& q, FormatContext& ctx) const
    {
        return formatter<std::string>::format(glm::to_string(q), ctx);
    }
};

namespace Engine
{
    namespace Utilities
    {
        // -------------------- Log --------------------

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

        // -------------------- Time --------------------

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

        // -------------------- Vulkan --------------------

        class VulkanUtilities
        {
        public:
            static void VKCheckStrict(VkResult result, const char* what);
        };

        // -------------------- File Management --------------------

        class FileManagement
        {
        public:
            static std::vector<char> LoadFromFile(const std::string& path);
        };
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

// Macros for color
#define TR_COLOR_GREY glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
#define TR_COLOR_RED glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
#define TR_COLOR_GREEN glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
#define TR_COLOR_BLUE glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);