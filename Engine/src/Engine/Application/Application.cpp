#include "Engine/Application/Application.h"
#include "Engine/Layer/Layer.h"

#include "Engine/Core/Utilities.h"

#include "Engine/Events/Input/Input.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/EventDispatcher.h"
#include "Engine/Events/WindowEvent.h"

namespace Engine
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        s_Instance = this;

        Utilities::Log::Initialize();
        Utilities::Time::Initialize();
        Input::Initialize();

        m_Window = Window::Create();
        m_Window->SetEventCallback([this](Event& e)
            {
                this->OnEvent(e);
            });

        Renderer::Initialize(*m_Window);
    }

    Application::~Application()
    {
        Renderer::Shutdown();
        Input::Shutdown();

        s_Instance = nullptr;
    }

    void Application::Close()
    {
        m_Running = false;
    }

    void Application::OnEvent(Event& e)
    {
        Input::OnEvent(e);

        EventDispatcher dispatcher(e);

        // Core engine events first
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& ev) { return OnWindowClose(ev); });
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResize(ev); });

        // Propagate to layers (top-most first)
        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            (*it)->OnEvent(e);
            if (e.Handled)
            {
                break;
            }
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent&)
    {
        m_Running = false;

        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            m_Minimized = true;

            return false;
        }

        m_Minimized = false;

        Renderer::OnResize((uint32_t)e.GetWidth(), (uint32_t)e.GetHeight());

        return false;
    }

    void Application::Run()
    {
        while (m_Running)
        {
            Utilities::Time::Update();

            Input::BeginFrame();
            m_Window->OnUpdate();

            if (m_Window->ShouldClose())
            {
                m_Running = false;
            }

            if (m_Minimized)
            {
                continue;
            }

            for (Layer* it_Layer : m_LayerStack)
            {
                it_Layer->OnUpdate(Utilities::Time::DeltaTime());
            }

            Renderer::BeginFrame();

            for (Layer* it_Layer : m_LayerStack)
            {
                it_Layer->OnRender();
            }

            Renderer::EndFrame();
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