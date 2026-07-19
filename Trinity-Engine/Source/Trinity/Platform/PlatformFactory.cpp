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
        return PlatformType::Windows;
#elif defined(TRINITY_PLATFORM_LINUX)
        return PlatformType::Linux;
#elif defined(TRINITY_PLATFORM_MACOS)
        return PlatformType::MacOS;
#else
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
                return std::make_unique<SDLPlatform>();
#else
                ("PlatformFactory: SDL backend disabled, no desktop platform available");
                return nullptr;
#endif
            }

            default:
                ("PlatformFactory: unsupported platform type");
                return nullptr;
        }
    }
}