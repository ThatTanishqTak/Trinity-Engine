#include <Trinity/Core/Application.h>

#include <Trinity/Core/Log.h>
#include <Trinity/Core/Assert.h>
#include <Trinity/Core/Timer.h>
#include <Trinity/Core/Engine.h>
#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Platform/Events/ApplicationEvent.h>

namespace Trinity
{
    Application* Application::s_Instance = nullptr;

    Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
    {
        TR_CORE_ASSERT(s_Instance == nullptr, "Application already exists");
        s_Instance = this;

        m_Engine = std::make_unique<Engine>();
    }

    Application::~Application()
    {
        s_Instance = nullptr;
    }

    void Application::Run()
    {
        TR_CORE_INFO("Starting '{}'", m_Specification.InternalName);

        if (!m_Engine->Initialize(m_Specification.InternalName))
        {
            TR_CORE_CRITICAL("Application: engine failed to initialize, aborting");
            return;
        }

        if (m_Specification.CreateWindowOnStartup)
        {
            m_Window = &m_Engine->GetPlatform().CreateWindow(m_Specification.Window);
            m_Window->SetEventCallback([this](Event& event) { OnEvent(event); });
        }

        OnInitialize();

        Timer l_FrameTimer;

        while (m_Running)
        {
            float l_Delta = l_FrameTimer.Elapsed();
            l_FrameTimer.Reset();

            Timestep l_Timestep(l_Delta);

            m_Engine->GetPlatform().PollEvents();

            if (!m_Minimized)
            {
                m_Engine->Update(l_Timestep);
                OnUpdate(l_Timestep);
            }
        }

        OnShutdown();
        m_Engine->Shutdown();

        TR_CORE_INFO("Stopped '{}'", m_Specification.InternalName);
    }

    void Application::Close()
    {
        m_Running = false;
    }

    void Application::OnEvent(Event& event)
    {
        EventDispatcher l_Dispatcher(event);
        l_Dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& closed) { return OnWindowClose(closed); });
        l_Dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& resized) { return OnWindowResize(resized); });

        OnEvent(event, event.IsHandled());
    }

    bool Application::OnWindowClose(WindowCloseEvent&)
    {
        m_Running = false;
        
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& event)
    {
        m_Minimized = (event.GetWidth() == 0 || event.GetHeight() == 0);

        return false;
    }
}