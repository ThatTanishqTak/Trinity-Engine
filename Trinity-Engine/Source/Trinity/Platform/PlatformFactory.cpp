#include <Trinity/Platform/PlatformFactory.h>

#include <Trinity/Core/Log.h>

#if defined(TRINITY_ENABLE_SDL)
#include <Trinity/Platform/Backends/SDL3/SDLPlatform.h>
#endif

namespace Trinity
{
    static PlatformType DetectPlatformType()
    {
#if defined(TRINITY_PLATFORM_WINDOWS)
        TR_CORE_TRACE("Platform: Windows");
        return PlatformType::Windows;
#elif defined(TRINITY_PLATFORM_LINUX)
        TR_CORE_TRACE("Platform: Linux");
        return PlatformType::Linux;
#elif defined(TRINITY_PLATFORM_MACOS)
        TR_CORE_TRACE("Platform: MacOS");
        return PlatformType::MacOS;
#else
        TR_CORE_WARN("Platform: Unknown");
        return PlatformType::Unknown;
#endif
    }

    std::unique_ptr<IPlatform> PlatformFactory::Create()
    {
        return Create(DetectPlatformType());
    }

    std::unique_ptr<IPlatform> PlatformFactory::Create(PlatformType type)
    {
        switch (type)
        {
            case PlatformType::Windows:
            case PlatformType::Linux:
            case PlatformType::MacOS:
            {
#if defined(TRINITY_ENABLE_SDL)
                TR_CORE_TRACE("Platform factory: SDL");
                return std::make_unique<SDLPlatform>();
#else
                TR_CORE_WARN("No platform factory available");
                return nullptr;
#endif
            }
            default:
                TR_CORE_CRITICAL("Failed to create platform factory");
                return nullptr;
        }
    }
}