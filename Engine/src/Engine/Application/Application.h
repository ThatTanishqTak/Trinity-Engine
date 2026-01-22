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

    private:
        bool m_Running = true;
        bool m_Minimized = false;

        LayerStack m_LayerStack;

        std::unique_ptr<Window> m_Window;
        std::unique_ptr<Renderer> m_Renderer;

        static Application* s_Instance;
    };
}