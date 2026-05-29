#include <Trinity/Core/Application.h>

#include <Trinity/Core/Log.h>
#include <Trinity/Core/Assert.h>
#include <Trinity/Core/Timer.h>
#include <Trinity/Core/Engine.h>
#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Platform/Events/ApplicationEvent.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanDevice.h>

namespace Trinity
{
    Application* Application::s_Instance = nullptr;

    Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
    {
        TR_CORE_ASSERT(s_Instance == nullptr, "[APPLICATION]: Application already exist");
        s_Instance = this;

        TR_CORE_INFO("[APPLICATION]: INITIALIZING APPLICATION");

        m_Engine = std::make_unique<Engine>();

        TR_CORE_INFO("[APPLICATION]: APPLICATION INITIALIZED");
    }

    Application::~Application()
    {
        s_Instance = nullptr;
    }

    void Application::Run()
    {
        if (!m_Engine->Initialize(m_Specification.InternalName))
        {
            TR_CORE_CRITICAL("[Application]: Failed to initialize engine");
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
                TR_CORE_CRITICAL("[Application]: Failed to initialized renderer");
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

                if (m_Engine->HasDevice() && m_Engine->GetDevice().HasSwapchain())
                {
                    if (m_SwapchainDirty)
                    {
                        m_Engine->GetDevice().GetSwapchain().Resize(m_PendingWidth, m_PendingHeight);
                        m_SwapchainDirty = false;
                    }

                    m_Engine->GetDevice().GetSwapchain().RenderFrame(0.05f, 0.05f, 0.05f);
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