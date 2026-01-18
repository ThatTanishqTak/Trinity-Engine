#pragma once

#include "Engine/Layer/LayerStack.h"
#include "Engine/Platform/Window.h"

#include <memory>
#include <chrono>

namespace Engine
{
    class Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);

        static Application& Get() { return *s_Instance; }

    private:
        bool m_Running = true;
        LayerStack m_LayerStack;

        std::unique_ptr<Window> m_Window;

        std::chrono::steady_clock::time_point m_LastFrameTime;

        static Application* s_Instance;
    };
}