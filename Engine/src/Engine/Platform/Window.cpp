#include "Engine/Utilities/Utilities.h"

#include "Engine/Platform/Window.h"
#include "Engine/Platform/GLFW/GLFWWindow.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/Event.h"

namespace Engine
{
    void Window::OnEvent(Event& e)
    {
        // Window dispatch keeps close and minimize state in the window itself.
        EventDispatcher l_Dispatcher(e);
        l_Dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });
        l_Dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
        l_Dispatcher.Dispatch<WindowFocusEvent>([this](WindowFocusEvent& e) { return OnWindowFocus(e); });
        l_Dispatcher.Dispatch<WindowLostFocusEvent>([this](WindowLostFocusEvent& e) { return OnWindowLostFocus(e); });
        l_Dispatcher.Dispatch<WindowMovedEvent>([this](WindowMovedEvent& e) { return OnWindowMoved(e); });
    }

    bool Window::OnWindowClose(WindowCloseEvent& e)
    {
        (void)e;
        m_ShouldClose = true;

        return true;
    }

    bool Window::OnWindowResize(WindowResizeEvent& e)
    {
        const uint32_t l_Width = e.GetWidth();
        const uint32_t l_Height = e.GetHeight();

        if (l_Width == 0 || l_Height == 0)
        {
            m_Minimized = true;

            return false;
        }

        m_Minimized = false;

        return false;
    }

    bool Window::OnWindowFocus(WindowFocusEvent& e)
    {
        (void)e;
        //TR_CORE_TRACE("{}", e.ToString());

        return false;
    }

    bool Window::OnWindowLostFocus(WindowLostFocusEvent& e)
    {
        (void)e;
        //TR_CORE_TRACE("{}", e.ToString());

        return false;
    }

    bool Window::OnWindowMoved(WindowMovedEvent& e)
    {
        (void)e;
        //TR_CORE_TRACE("{}", e.ToString());

        return false;
    }

    std::unique_ptr<Window> Window::Create(const WindowProperties& properties)
    {
        TR_CORE_TRACE("Creating window");

        TR_CORE_TRACE("Title: {}", properties.Title);
        TR_CORE_TRACE("Width: {}", properties.Width);
        TR_CORE_TRACE("Height: {}", properties.Height);
        TR_CORE_TRACE("VSync: {}", properties.VSync);
        
        TR_CORE_TRACE("Window created");

        return std::make_unique<GLFWWindow>(properties);
    }
}