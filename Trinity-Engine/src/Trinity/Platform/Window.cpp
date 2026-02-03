#include "Trinity/Platform/Window.h"
#include "Trinity/Utilities/Utilities.h"

#ifdef _WIN32
#include "Trinity/Platform/Windows/WindowsWindow.h"
#else
#error Native window backend not implemented for this platform yet.
#endif

namespace Trinity
{
    std::unique_ptr<Window> Window::Create()
    {
#ifdef _WIN32
        return std::make_unique<WindowsWindow>();
#else
        TR_CORE_CRITICAL("Unsupported platform for Trinity::Window::Create()");
        
        return nullptr;
#endif
    }
}