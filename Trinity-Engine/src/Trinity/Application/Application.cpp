#include "Trinity/Application/Application.h"
#include "Trinity/Layer/Layer.h"
#include "Trinity/ImGui/ImGuiLayer.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/Time.h"

#include "Trinity/Platform/Window.h"

#include "Trinity/Events/Event.h"
#include "Trinity/Events/EventQueue.h"
#include "Trinity/Events/ApplicationEvent.h"
#include "Trinity/Events/KeyEvent.h"
#include "Trinity/Events/MouseEvent.h"
#include "Trinity/Events/GamepadEvent.h"

#include "Trinity/Input/Input.h"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>
#include <utility>

namespace Trinity
{
    Application* Application::s_Instance = nullptr;
    bool Application::s_Running = true;

    Application::Application(ApplicationSpecification& specification) : m_Specification(specification)
    {
        TR_CORE_INFO("------- INITIALIZING APPLICATION -------");

        if (s_Instance != nullptr)
        {
            TR_CORE_CRITICAL("Application instance already exists.");

            std::abort();
        }

        s_Instance = this;

        WindowProperties l_WindowProperties;
        l_WindowProperties.Title = m_Specification.Title;
        l_WindowProperties.Width = m_Specification.Width;
        l_WindowProperties.Height = m_Specification.Height;

        m_Window = Window::Create();
        m_Window->Initialize(l_WindowProperties);

        RenderCommand::Initialize(*m_Window, RendererAPI::VULKAN);

        m_ImGuiLayer = std::make_unique<ImGuiLayer>();
        m_ImGuiLayer->Initialize(*m_Window);

        TR_CORE_INFO("------- APPLICATION INITIALIZED -------");
    }

    Application::~Application()
    {
        TR_CORE_INFO("------- SHUTTING DOWN APPLICATION -------");

        m_ImGuiLayer->Shutdown();
        m_ImGuiLayer.reset();

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
        m_ImGuiLayer->OnEvent(e);

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

        m_Window->OnEvent(e);
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

            auto& l_EventQueue = m_Window->GetEventQueue();
            std::unique_ptr<Event> l_Event;
            while (l_EventQueue.TryPopEvent(l_Event))
            {
                OnEvent(*l_Event);
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

            m_ImGuiLayer->BeginFrame();

            for (const std::unique_ptr<Layer>& it_Layer : m_LayerStack)
            {
                it_Layer->OnImGuiRender();
            }

            m_ImGuiLayer->EndFrame();

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