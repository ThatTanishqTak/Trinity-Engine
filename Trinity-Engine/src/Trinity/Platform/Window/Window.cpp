#include "Trinity/Platform/Window/Window.h"

#if defined(TRINITY_PLATFORM_DESKTOP)
#include "Trinity/Platform/Window/Desktop/DesktopWindow.h"
#elif defined(TRINITY_PLATFORM_PLAYSTATION)
#include "Trinity/Platform/Window/PlayStation/PlayStationWindow.h"
#elif defined(TRINITY_PLATFORM_XBOX)
#include "Trinity/Platform/Window/Xbox/XboxWindow.h"
#elif defined(TRINITY_PLATFORM_NINTENDO)
#include "Trinity/Platform/Window/Nintendo/NintendoWindow.h"
#endif

namespace Trinity
{
	std::unique_ptr<Window> Window::Create()
	{
#if defined(TRINITY_PLATFORM_DESKTOP)
		return std::make_unique<DesktopWindow>();
#elif defined(TRINITY_PLATFORM_PLAYSTATION)
		return std::make_unique<PlayStationWindow>();
#elif defined(TRINITY_PLATFORM_XBOX)
		return std::make_unique<XboxWindow>();
#elif defined(TRINITY_PLATFORM_NINTENDO)
		return std::make_unique<NintendoWindow>();
#else
#error No platform defined for Window::Create()
#endif
	}
}