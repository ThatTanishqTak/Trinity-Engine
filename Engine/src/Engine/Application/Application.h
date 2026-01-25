#pragma once

#include "Engine/Layer/LayerStack.h"
#include "Engine/Platform/Window.h"

#include <memory>

namespace Engine
{
    class Renderer;

    class Event;

    class Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
        void Close();

        void PushLayer(std::unique_ptr<Layer> layer);
        void PushOverlay(std::unique_ptr<Layer> overlay);

        static Application& Get() { return *s_Instance; }

    private:
        void OnEvent(Event& e);

    private:
        bool m_Running = true;
        LayerStack m_LayerStack;

        std::unique_ptr<Window> m_Window;
        std::unique_ptr<Renderer> m_Renderer;

        static Application* s_Instance;
    };
}