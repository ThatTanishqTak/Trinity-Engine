#include "Engine/Application/Application.h"
#include "Engine/Layer/Layer.h"

#include "Engine/Utilities/Utilities.h"
#include "Engine/Renderer/Renderer.h"

#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/GamepadEvent.h"

namespace Engine
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        s_Instance = this;

        Utilities::Log::Initialize();
        Utilities::Time::Initialize();

        m_Window = Window::Create();
        m_Window->SetEventCallback([this](Event& e)
        {
            OnEvent(e);
        });

        // Renderer lives as long as the window lives
        m_Renderer = std::make_unique<Renderer>();
        m_Renderer->Initialize(*m_Window);
    }

    Application::~Application()
    {
        // Make destruction order explicit
        m_Renderer.reset();
        s_Instance = nullptr;
    }

    void Application::Close()
    {
        m_Running = false;
    }

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& ev) { return OnWindowClose(ev); });
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResize(ev); });

        // Layers get events from top to bottom (overlays first).
        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
        {
            (*--it)->OnEvent(e);
            if (e.Handled)
            {
                TR_CORE_TRACE(e.ToString());
                break;
            }
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        (void)e;
        m_Running = false;
        
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        const uint32_t l_Width = e.GetWidth();
        const uint32_t l_Height = e.GetHeight();

        if (l_Width == 0 || l_Height == 0)
        {
            m_Minimized = true;
            
            return false;
        }

        m_Minimized = false;

        if (m_Renderer)
        {
            m_Renderer->OnResize(l_Width, l_Height);
        }

        return false;
    }

    void Application::Run()
    {
        while (m_Running)
        {
            Utilities::Time::Update();

            // Poll events
            m_Window->OnUpdate();

            if (!m_Running)
            {
                break;
            }

            if (m_Window->ShouldClose())
            {
                m_Running = false;
 
                break;
            }

            if (m_Minimized)
            {
                continue;
            }

            for (Layer* it_Layer : m_LayerStack)
            {
                it_Layer->OnUpdate(Utilities::Time::DeltaTime());
            }

            m_Renderer->BeginFrame();

            for (Layer* it_Layer : m_LayerStack)
            {
                it_Layer->OnRender();
            }

            m_Renderer->EndFrame();
        }
    }

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.PushLayer(layer);
    }

    void Application::PushOverlay(Layer* overlay)
    {
        m_LayerStack.PushOverlay(overlay);
    }
}