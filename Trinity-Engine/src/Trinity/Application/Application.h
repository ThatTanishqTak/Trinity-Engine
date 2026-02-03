#pragma once

#include "Trinity/Layer/LayerStack.h"

#include <memory>

namespace Trinity
{
    class Window;
    class Event;
    class VulkanRenderer;

    class Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
        static void Close();

        void PushLayer(std::unique_ptr<Layer> layer);
        void PushOverlay(std::unique_ptr<Layer> overlay);

        static Application& Get();
        Window& GetWindow();

    private:
        void OnEvent(Event& e);

    private:
        static bool s_Running;

        LayerStack m_LayerStack;
        std::unique_ptr<Window> m_Window;
        std::unique_ptr<VulkanRenderer> m_Renderer;

        static Application* s_Instance;
    };
}