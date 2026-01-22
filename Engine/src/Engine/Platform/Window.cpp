#include "Engine/Utilities/Utilities.h"

#include "Engine/Platform/Window.h"
#include "Engine/Platform/GLFW/GLFWWindow.h"

namespace Engine
{
    std::unique_ptr<Window> Window::Create(const WindowProperties& properties)
    {
        return std::make_unique<GLFWWindow>(properties);
    }
}