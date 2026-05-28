#pragma once

#include <Trinity/Core/Log.h>

#if defined(TRINITY_PLATFORM_WINDOWS)
#define TR_DEBUGBREAK() __debugbreak()
#elif defined(TRINITY_PLATFORM_LINUX)
#include <csignal>
#define TR_DEBUGBREAK() raise(SIGTRAP)
#elif defined(TRINITY_PLATFORM_MACOS)
#define TR_DEBUGBREAK() __builtin_trap()
#else
#define TR_DEBUGBREAK()
#endif

#ifdef TRINITY_DEBUG
#define TR_ENABLE_ASSERTS
#endif

#ifdef TR_ENABLE_ASSERTS
#define TR_INTERNAL_BREAK() TR_DEBUGBREAK()
#else
#define TR_INTERNAL_BREAK() ((void)0)
#endif

#ifdef TR_ENABLE_ASSERTS

#define TR_CORE_ASSERT(condition, ...) \
        do \
        { \
            if (!(condition)) \
            { \
                TR_CORE_CRITICAL("Assertion failed: ({}) in {}:{}", #condition, __FILE__, __LINE__); \
                __VA_OPT__(TR_CORE_CRITICAL(__VA_ARGS__);) \
                TR_DEBUGBREAK(); \
            } \
        } while (0)

#define TR_ASSERT(condition, ...) \
        do \
        { \
            if (!(condition)) \
            { \
                TR_CRITICAL("Assertion failed: ({}) in {}:{}", #condition, __FILE__, __LINE__); \
                __VA_OPT__(TR_CRITICAL(__VA_ARGS__);) \
                TR_DEBUGBREAK(); \
            } \
        } while (0)

#else
#define TR_CORE_ASSERT(condition, ...) ((void)0)
#define TR_ASSERT(condition, ...) ((void)0)
#endif

#define TR_CORE_VERIFY(condition, ...) \
    do \
    { \
        if (!(condition)) \
        { \
            TR_CORE_ERROR("Verify failed: ({}) in {}:{}", #condition, __FILE__, __LINE__); \
            __VA_OPT__(TR_CORE_ERROR(__VA_ARGS__);) \
            TR_INTERNAL_BREAK(); \
        } \
    } while (0)

#define TR_VERIFY(condition, ...) \
    do \
    { \
        if (!(condition)) \
        { \
            TR_ERROR("Verify failed: ({}) in {}:{}", #condition, __FILE__, __LINE__); \
            __VA_OPT__(TR_ERROR(__VA_ARGS__);) \
            TR_INTERNAL_BREAK(); \
        } \
    } while (0)