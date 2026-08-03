#pragma once

#include <string>
#include <memory>

#include <Trinity/Core/SimulationClock.h>
#include <Trinity/Core/Timestep.h>
#include <Trinity/Platform/Window.h>

namespace Trinity
{
    class Engine;
    class Event;
    class WindowCloseEvent;
    class WindowResizeEvent;

    struct CommandLineArgs
    {
        int Count = 0;
        char** Args = nullptr;

        const char* operator[](int index) const
        {
            return Args[index];
        }
    };

    struct ApplicationSpecification
    {
        std::string InternalName = "Trinity Application";

        bool CreateWindowOnStartup = true;

        float FixedDelta = SimulationClock::DefaultFixedDelta;
        uint32_t MaxSubSteps = SimulationClock::DefaultMaxSubSteps;

        WindowProperties Window;
        CommandLineArgs Arguments;
    };

    class Application
    {
    public:
        Application(const ApplicationSpecification& specification);
        virtual ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        void Run();
        void Close();

        void OnEvent(Event& event);

        const ApplicationSpecification& GetSpecification() const { return m_Specification; }
        Engine& GetEngine() { return *m_Engine; }
        Window& GetWindow() { return *m_Window; }
        bool HasWindow() const { return m_Window != nullptr; }

        static Application& Get() { return *s_Instance; }

    protected:
        virtual void OnInitialize()
        {

        }

        virtual void OnFixedUpdate(Timestep)
        {

        }

        virtual void OnUpdate(Timestep)
        {

        }

        virtual void OnImGuiRender()
        {

        }

        virtual void OnShutdown()
        {

        }
        
        virtual void OnEvent(Event&, bool)
        {

        }

    private:
        bool OnWindowClose(WindowCloseEvent& event);
        bool OnWindowResize(WindowResizeEvent& event);

    private:
        ApplicationSpecification m_Specification;
        std::unique_ptr<Engine> m_Engine;
        Window* m_Window = nullptr;

        bool m_Running = true;
        bool m_Minimized = false;
        bool m_SwapchainDirty = false;

        float m_StepClampWarnCooldown = 0.0f;

        uint32_t m_PendingWidth = 0;
        uint32_t m_PendingHeight = 0;

        static Application* s_Instance;
    };

    Application* CreateApplication(CommandLineArgs args);
}