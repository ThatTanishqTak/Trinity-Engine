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

#include <chrono>
#include <cstdlib>
#include <thread>
#include <utility>

namespace Engine
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        TR_CORE_INFO("------- INITIALIZING APPLICATION -------");

        if (s_Instance != nullptr)
        {
            TR_CORE_CRITICAL("Application instance already exists.");

            std::abort();
        }

        s_Instance = this;

        m_Window = Window::Create();
        m_Window->SetEventCallback([this](Event& e)
        {
            OnEvent(e);
        });

        // Renderer starts after window exists.
        Render::Renderer::Initialize(m_Window.get());

        TR_CORE_INFO("------- APPLICATION INITIALIZED -------");
    }

    Application::~Application()
    {
        TR_CORE_INFO("------- SHUTTING DOWN APPLICATION -------");

        Render::Renderer::Shutdown();

        m_LayerStack.Shutdown();
        m_Window.reset();

        s_Instance = nullptr;

        TR_CORE_INFO("------- APPLICATION SHUTDOWN COMPLETE -------");
    }

    Application& Application::Get()
    {
        if (s_Instance == nullptr)
        {
            TR_CORE_CRITICAL("Application instance not available.");

            std::abort();
        }

        return *s_Instance;
    }

    void Application::Close()
    {
        m_Running = false;
    }

    void Application::OnEvent(Event& e)
    {
        Input::OnEvent(e);
        m_Window->OnEvent(e);

        // Let renderer know about resizes.
        if (e.GetEventType() == EventType::WindowResize)
        {
            auto& l_Resize = static_cast<WindowResizeEvent&>(e);
            Render::Renderer::OnResize(l_Resize.GetWidth(), l_Resize.GetHeight());
        }

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
                constexpr auto l_MinimizedSleep = std::chrono::milliseconds(16);

                std::this_thread::sleep_for(l_MinimizedSleep);
                m_Window->OnUpdate();

                continue;
            }

            for (const std::unique_ptr<Layer>& it_Layer : m_LayerStack)
            {
                it_Layer->OnUpdate(Utilities::Time::DeltaTime());
            }

            const bool l_CanRender = Render::Renderer::BeginFrame();
            if (l_CanRender)
            {
                for (const std::unique_ptr<Layer>& it_Layer : m_LayerStack)
                {
                    it_Layer->OnRender();
                }

                for (const std::unique_ptr<Layer>& it_Layer : m_LayerStack)
                {
                    it_Layer->OnImGuiRender();
                }

                Render::Renderer::EndFrame();
            }
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