#include "Trinity/Platform/Window/Window.h"

#include "Trinity/Platform/Window/Desktop/DesktopWindow.h"
#include "Trinity/Platform/Window/PlayStation/PlayStationWindow.h"
#include "Trinity/Platform/Window/Xbox/XboxWindow.h"
#include "Trinity/Platform/Window/Nintendo/NintendoWindow.h"

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