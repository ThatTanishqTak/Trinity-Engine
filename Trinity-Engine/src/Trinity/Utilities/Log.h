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

namespace Trinity
{
    namespace CoreUtilities
    {
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
    }
}

// Core log macros
#define TR_CORE_TRACE(...) ::Trinity::CoreUtilities::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define TR_CORE_DEBUG(...) ::Trinity::CoreUtilities::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define TR_CORE_INFO(...)  ::Trinity::CoreUtilities::Log::GetCoreLogger()->info(__VA_ARGS__)
#define TR_CORE_WARN(...)  ::Trinity::CoreUtilities::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define TR_CORE_ERROR(...) ::Trinity::CoreUtilities::Log::GetCoreLogger()->error(__VA_ARGS__)
#define TR_CORE_CRITICAL(...) ::Trinity::CoreUtilities::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define TR_TRACE(...) ::Trinity::CoreUtilities::Log::GetClientLogger()->trace(__VA_ARGS__)
#define TR_DEBUG(...) ::Trinity::CoreUtilities::Log::GetClientLogger()->debug(__VA_ARGS__)
#define TR_INFO(...)  ::Trinity::CoreUtilities::Log::GetClientLogger()->info(__VA_ARGS__)
#define TR_WARN(...)  ::Trinity::CoreUtilities::Log::GetClientLogger()->warn(__VA_ARGS__)
#define TR_ERROR(...) ::Trinity::CoreUtilities::Log::GetClientLogger()->error(__VA_ARGS__)
#define TR_CRITICAL(...) ::Trinity::CoreUtilities::Log::GetClientLogger()->critical(__VA_ARGS__)