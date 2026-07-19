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
        TR_CORE_ASSERT(s_Instance == nullptr, "Application already exist");
        s_Instance = this;



        m_Engine = std::make_unique<Engine>();


    }

    Application::~Application()
    {
        s_Instance = nullptr;
    }

    void Application::Run()
    {
        if (!m_Engine->Initialize(m_Specification.InternalName))
        {

            return;
        }

        if (m_Specification.CreateWindowOnStartup)
        {
            m_Window = &m_Engine->GetPlatform().CreateWindow(m_Specification.Window);
            m_Window->SetEventCallback([this](Event& event)
            {
                OnEvent(event);
            });

            if (!m_Engine->InitializeRenderer(m_Window->GetNativeHandle(), m_Specification.InternalName))
            {

                return;
            }
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

                if (m_Engine->HasRenderer())
                {
                    if (m_SwapchainDirty)
                    {
                        m_Engine->Resize(m_PendingWidth, m_PendingHeight);
                        m_SwapchainDirty = false;
                    }

                    m_Engine->BeginImGuiFrame();

                    OnImGuiRender();
                    
                    m_Engine->RenderFrame();
                }
            }
        }

        OnShutdown();
        m_Engine->Shutdown();
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
        const uint32_t l_Width = event.GetWidth();
        const uint32_t l_Height = event.GetHeight();

        m_Minimized = (l_Width == 0 || l_Height == 0);

        if (!m_Minimized)
        {
            m_PendingWidth = l_Width;
            m_PendingHeight = l_Height;

            m_SwapchainDirty = true;
        }

        return false;
    }
}