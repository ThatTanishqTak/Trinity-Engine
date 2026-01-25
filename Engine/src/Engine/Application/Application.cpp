#include "Engine/Application/Application.h"
#include "Engine/Layer/Layer.h"

#include "Engine/Utilities/Utilities.h"
#include "Engine/Renderer/Renderer.h"

#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/GamepadEvent.h"

#include "Engine/Input/Input.h"

#include <utility>

namespace Engine
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        TR_CORE_INFO("INITIALIZING APPLICATION");

        s_Instance = this;

        m_Window = Window::Create();
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
        TR_CORE_INFO("SHUTTING DOWN APPLICATION");

        m_LayerStack.Shutdown();

        m_Renderer.reset();
        m_Window.reset();

        s_Instance = nullptr;

        TR_CORE_INFO("APPLICATION SHUTDOWN COMPLETE");
    }

    void Application::Close()
    {
        m_Running = false;
    }

    void Application::OnEvent(Event& e)
    {
        Input::OnEvent(e);

        m_Window->OnEvent(e);

        if (e.GetEventType() == EventType::WindowResize)
        {
            const auto& l_ResizeEvent = static_cast<const WindowResizeEvent&>(e);

            if (!m_Window->IsMinimized() && m_Renderer)
            {
                m_Renderer->OnResize(l_ResizeEvent.GetWidth(), l_ResizeEvent.GetHeight());
            }
        }

        // Layers get events from top to bottom
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

    void Application::Run()
    {
        while (m_Running)
        {
            Utilities::Time::Update();

            Input::BeginFrame();

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

            if (m_Window->IsMinimized())
            {
                continue;
            }

            for (const std::unique_ptr<Layer>& it_Layer : m_LayerStack)
            {
                it_Layer->OnUpdate(Utilities::Time::DeltaTime());
            }

            m_Renderer->BeginFrame();

            for (const std::unique_ptr<Layer>& it_Layer : m_LayerStack)
            {
                it_Layer->OnRender();
            }

            m_Renderer->EndFrame();
        }
    }

    void Application::PushLayer(std::unique_ptr<Layer> layer)
    {
        m_LayerStack.PushLayer(std::move(layer));
    }

    void Application::PushOverlay(std::unique_ptr<Layer> overlay)
    {
        m_LayerStack.PushOverlay(std::move(overlay));
    }
}