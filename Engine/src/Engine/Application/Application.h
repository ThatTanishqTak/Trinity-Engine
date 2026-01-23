#pragma once

#include "Engine/Layer/LayerStack.h"
#include "Engine/Platform/Window.h"

#include <memory>

namespace Engine
{
    class Renderer;

    class Event;
    class WindowCloseEvent;
    class WindowResizeEvent;
    class KeyEvent;
    class KeyPressedEvent;
    class KeyReleasedEvent;
    class KeyTypedEvent;
    class MouseMovedEvent;
    class MouseScrolledEvent;
    class MouseButtonPressedEvent;
    class MouseButtonReleasedEvent;

    class Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
        void Close();

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);

        static Application& Get() { return *s_Instance; }

    private:
        void OnEvent(Event& e);

        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);
        bool OnKeyPressed(KeyPressedEvent& e);
        bool OnKeyReleased(KeyReleasedEvent& e);
        bool OnKeyTyped(KeyTypedEvent& e);
        bool OnMouseMoved(MouseMovedEvent& e);
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
        bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);

    private:
        bool m_Running = true;
        bool m_Minimized = false;

        LayerStack m_LayerStack;

        std::unique_ptr<Window> m_Window;
        std::unique_ptr<Renderer> m_Renderer;

        static Application* s_Instance;
    };
}