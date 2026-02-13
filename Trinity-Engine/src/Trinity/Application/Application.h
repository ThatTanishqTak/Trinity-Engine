#pragma once

#include "Trinity/Layer/LayerStack.h"
#include "Trinity/Renderer/RenderCommand.h"

#include <cstdint>
#include <memory>
#include <string>

namespace Trinity
{
    class Window;
    class Event;

    struct ApplicationSpecification
    {
        std::string Title = "Trinity-Application";
        uint32_t Width = 1280;
        uint32_t Height = 720;
    };

    class Application
    {
    public:
        Application(ApplicationSpecification& specification);
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
        ApplicationSpecification m_Specification;

        std::unique_ptr<Window> m_Window;

        static Application* s_Instance;
    };
}