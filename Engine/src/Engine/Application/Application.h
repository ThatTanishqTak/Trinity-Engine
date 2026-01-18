#pragma once

#include "Engine/Layer/LayerStack.h"
#include "Engine/Platform/Window.h"

#include <memory>

namespace Engine
{
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

        void OnEvent(Event& e);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);

        static Application& Get() { return *s_Instance; }

    private:
        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

    private:
        bool m_Running = true;
        bool m_Minimized = false;

        LayerStack m_LayerStack;
        std::unique_ptr<Window> m_Window;

        static Application* s_Instance;
    };
}