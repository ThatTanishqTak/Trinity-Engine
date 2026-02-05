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
#include <filesystem>
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

namespace Trinity
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
            static void VKCheck(VkResult result, const char* what);
        };

        // -------------------- File Management --------------------

        class FileManagement
        {
        public:
            static std::vector<char> LoadFromFile(const std::string& path);
            static void SaveToFile(const std::string& path, const std::vector<char>& data);
        };
    }
}

// Core log macros
#define TR_CORE_TRACE(...) ::Trinity::Utilities::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define TR_CORE_INFO(...)  ::Trinity::Utilities::Log::GetCoreLogger()->info(__VA_ARGS__)
#define TR_CORE_WARN(...)  ::Trinity::Utilities::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define TR_CORE_ERROR(...) ::Trinity::Utilities::Log::GetCoreLogger()->error(__VA_ARGS__)
#define TR_CORE_CRITICAL(...) ::Trinity::Utilities::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define TR_TRACE(...) ::Trinity::Utilities::Log::GetClientLogger()->trace(__VA_ARGS__)
#define TR_INFO(...)  ::Trinity::Utilities::Log::GetClientLogger()->info(__VA_ARGS__)
#define TR_WARN(...)  ::Trinity::Utilities::Log::GetClientLogger()->warn(__VA_ARGS__)
#define TR_ERROR(...) ::Trinity::Utilities::Log::GetClientLogger()->error(__VA_ARGS__)
#define TR_CRITICAL(...) ::Trinity::Utilities::Log::GetClientLogger()->critical(__VA_ARGS__)

// Macros for color
#define TR_COLOR_GREY glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
#define TR_COLOR_RED glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
#define TR_COLOR_GREEN glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
#define TR_COLOR_BLUE glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

// -------------------- Diagnostics --------------------

#if defined(_MSC_VER)
#define TR_DEBUGBREAK() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__)
#define TR_DEBUGBREAK() raise(SIGTRAP)
#else
#define TR_DEBUGBREAK() ((void)0)
#endif

#define TR_ABORT() do { TR_DEBUGBREAK(); std::abort(); } while (0)

#ifdef TRINITY_DEBUG
#define TR_ENABLE_ASSERTS 1
#else
#define TR_ENABLE_ASSERTS 0
#endif

#if TR_ENABLE_ASSERTS

#define TR_CORE_ASSERT(condition, ...) \
        do { \
            if (!(condition)) { \
                TR_CORE_CRITICAL("Assertion failed: {} ({}:{})", #condition, __FILE__, __LINE__); \
                TR_CORE_CRITICAL(__VA_ARGS__); \
                TR_ABORT(); \
            } \
        } while (0)

#define TR_ASSERT(condition, ...) \
        do { \
            if (!(condition)) { \
                TR_CRITICAL("Assertion failed: {} ({}:{})", #condition, __FILE__, __LINE__); \
                TR_CRITICAL(__VA_ARGS__); \
                TR_ABORT(); \
            } \
        } while (0)

#define TR_CORE_VERIFY(condition, ...) TR_CORE_ASSERT(condition, __VA_ARGS__)
#define TR_VERIFY(condition, ...) TR_ASSERT(condition, __VA_ARGS__)

#else

#define TR_CORE_ASSERT(condition, ...) ((void)0)
#define TR_ASSERT(condition, ...) ((void)0)

#define TR_CORE_VERIFY(condition, ...) \
        do { \
            if (!(condition)) { \
                TR_CORE_ERROR("Verify failed: {} ({}:{})", #condition, __FILE__, __LINE__); \
                TR_CORE_ERROR(__VA_ARGS__); \
            } \
        } while (0)

#define TR_VERIFY(condition, ...) \
        do { \
            if (!(condition)) { \
                TR_ERROR("Verify failed: {} ({}:{})", #condition, __FILE__, __LINE__); \
                TR_ERROR(__VA_ARGS__); \
            } \
        } while (0)

#endif

#define TR_CORE_FATAL(...) do { TR_CORE_CRITICAL(__VA_ARGS__); TR_ABORT(); } while (0)
#define TR_FATAL(...) do { TR_CRITICAL(__VA_ARGS__); TR_ABORT(); } while (0)