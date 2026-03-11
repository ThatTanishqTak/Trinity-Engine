#include "Trinity/Platform/Window.h"

#include "Trinity/Platform/Windows/WindowsWindow.h"

namespace Trinity
{
    std::unique_ptr<Window> Window::Create()
    {
        return std::make_unique<WindowsWindow>();
    }
}