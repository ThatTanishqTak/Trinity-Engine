#include "Trinity/Application/Application.h"

#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Layer/Layer.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/Time.h"

#include "Trinity/Platform/Window/Window.h"

#include "Trinity/Events/ApplicationEvent.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Events/EventQueue.h"

#include "Trinity/Platform/Input/Desktop/DesktopInput.h"

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Asset/AssetRegistry.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>
#include <utility>

namespace Trinity
{
    Application* Application::s_Instance = nullptr;
    std::atomic<bool> Application::s_Running = true;

    Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
    {
        TR_CORE_INFO("------- INITIALIZING APPLICATION -------");

        if (s_Instance != nullptr)
        {
            TR_CORE_CRITICAL("Application instance already exists");

            std::abort();
        }

        s_Instance = this;
        s_Running = true;

        WindowProperties l_WindowProperties;
        l_WindowProperties.Title = m_Specification.Title;
        l_WindowProperties.Width = m_Specification.Width;
        l_WindowProperties.Height = m_Specification.Height;

        m_Window = Window::Create();
        m_Window->Initialize(l_WindowProperties);

        RendererSpecification l_RendererSpecification;
        l_RendererSpecification.Backend = RendererBackend::Vulkan;
        l_RendererSpecification.MaxFramesInFlight = 2;
#ifdef TRINITY_DEBUG
        l_RendererSpecification.EnableValidation = true;
#else
        l_RendererSpecification.EnableValidation = false;
#endif

        Renderer::Initialize(*m_Window, l_RendererSpecification);

        auto a_ImGuiLayer = std::make_unique<ImGuiLayer>();
        m_ImGuiLayer = a_ImGuiLayer.get();
        PushOverlay(std::move(a_ImGuiLayer));

        TR_CORE_INFO("------- APPLICATION INITIALIZED -------");
    }

    Application::~Application()
    {
        TR_CORE_INFO("------- SHUTTING DOWN APPLICATION -------");

        Renderer::WaitIdle();

        m_LayerStack.Shutdown();

        AssetRegistry::Get().Clear();

        Renderer::Shutdown();

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
        DesktopInput::OnEvent(e);

        EventDispatcher l_Dispatcher(e);
        l_Dispatcher.Dispatch<WindowResizeEvent>([](WindowResizeEvent& resize) -> bool
        {
            Renderer::OnWindowResize(resize.GetWidth(), resize.GetHeight());

            return false;
        });

        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
        {
            (*--it)->OnEvent(e);
            if (e.Handled)
            {
                break;
            }
        }

        m_Window->OnEvent(e);
    }

    void Application::Run()
    {
        while (s_Running)
        {
            CoreUtilities::Time::Update();
            DesktopInput::BeginFrame();

            m_Window->OnUpdate();

            if (!s_Running)
            {
                break;
            }

            auto& a_EventQueue = m_Window->GetEventQueue();
            std::unique_ptr<Event> l_Event;
            while (a_EventQueue.TryPopEvent(l_Event))
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
                constexpr auto a_MinimizedSleep = std::chrono::milliseconds(16);
                std::this_thread::sleep_for(a_MinimizedSleep);
            }

            for (const std::unique_ptr<Layer>& it_Layer : m_LayerStack)
            {
                it_Layer->OnUpdate(CoreUtilities::Time::DeltaTime());
            }

            if (Renderer::BeginFrame())
            {
                for (const std::unique_ptr<Layer>& it_Layer : m_LayerStack)
                {
                    it_Layer->OnRender();
                }

                assert(m_ImGuiLayer && "ImGuiLayer not initialized");
                m_ImGuiLayer->Begin();

                for (const std::unique_ptr<Layer>& it_Layer : m_LayerStack)
                {
                    it_Layer->OnImGuiRender();
                }

                m_ImGuiLayer->End();

                Renderer::EndFrame();
                Renderer::Present();
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