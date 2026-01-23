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
        TR_CORE_INFO("INITIALIZING APPLICATION");

        s_Instance = this;

        m_Window = Window::Create();
        m_Window->Initialize();
        m_Window->SetEventCallback([this](Event& e)
        {
            OnEvent(e);
        });

        m_Renderer = Renderer::Create();
        m_Renderer->Initialize(*m_Window);

        TR_CORE_INFO("APPLICATION INITIALIZED");
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
        dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {return OnKeyPressed(ev); });
        dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& ev) {return OnKeyReleased(ev); });
        dispatcher.Dispatch<KeyTypedEvent>([this](KeyTypedEvent& ev) {return OnKeyTyped(ev); });
        dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& ev) {return OnMouseMoved(ev); });
        dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) {return OnMouseScrolled(ev); });
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) {return OnMouseButtonPressed(ev); });
        dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& ev) {return OnMouseButtonReleased(ev); });

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

    bool Application::OnKeyPressed(KeyPressedEvent& e)
    {
        TR_CORE_TRACE("{}", e.ToString());

        return false;
    }

    bool Application::OnKeyReleased(KeyReleasedEvent& e)
    {
        TR_CORE_TRACE("{}", e.ToString());

        return false;
    }

    bool Application::OnKeyTyped(KeyTypedEvent& e)
    {
        TR_CORE_TRACE("{}", e.ToString());

        return false;
    }

    bool Application::OnMouseMoved(MouseMovedEvent& e)
    {
        TR_CORE_TRACE("{}", e.ToString());

        return false;
    }

    bool Application::OnMouseScrolled(MouseScrolledEvent& e)
    {
        TR_CORE_TRACE("{}", e.ToString());

        return false;
    }

    bool Application::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        TR_CORE_TRACE("{}", e.ToString());

        return false;
    }

    bool Application::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
    {
        TR_CORE_TRACE("{}", e.ToString());

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