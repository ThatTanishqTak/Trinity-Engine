#include "Trinity/Application/Application.h"
#include "Trinity/Layer/Layer.h"

#include "Trinity/Utilities/Utilities.h"

#include "Trinity/Platform/Window.h"

#include "Trinity/Events/Event.h"
#include "Trinity/Events/ApplicationEvent.h"
#include "Trinity/Events/KeyEvent.h"
#include "Trinity/Events/MouseEvent.h"
#include "Trinity/Events/GamepadEvent.h"

#include "Trinity/Input/Input.h"

#include <chrono>
#include <cstdlib>
#include <thread>
#include <utility>

namespace Trinity
{
    Application* Application::s_Instance = nullptr;
    bool Application::s_Running = true;

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
        m_Window->Initialize();
        m_Window->SetEventCallback([this](Event& e)
        {
            OnEvent(e);
        });

        RenderCommand::Initialize(*m_Window, RendererAPI::VULKAN);

        TR_CORE_INFO("------- APPLICATION INITIALIZED -------");
    }

    Application::~Application()
    {
        TR_CORE_INFO("------- SHUTTING DOWN APPLICATION -------");

        RenderCommand::Shutdown();

        m_LayerStack.Shutdown();

        m_Window->Shutdown();
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
        s_Running = false;
    }

    Window& Application::GetWindow()
    {
        return *m_Window;
    }

    void Application::OnEvent(Event& e)
    {
        Input::OnEvent(e);
        m_Window->OnEvent(e);

        if (e.GetEventType() == EventType::WindowResize)
        {
            auto& l_Resize = static_cast<WindowResizeEvent&>(e);
            RenderCommand::Resize(l_Resize.GetWidth(), l_Resize.GetHeight());
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
        while (s_Running)
        {
            Utilities::Time::Update();
            Input::BeginFrame();

            m_Window->OnUpdate();

            if (!s_Running)
            {
                break;
            }

            if (m_Window->ShouldClose())
            {
                s_Running = false;

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

            RenderCommand::BeginFrame();

            for (const std::unique_ptr<Layer>& it_Layer : m_LayerStack)
            {
                it_Layer->OnRender();
            }

            for (const std::unique_ptr<Layer>& it_Layer : m_LayerStack)
            {
                it_Layer->OnImGuiRender();
            }

            RenderCommand::EndFrame();
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