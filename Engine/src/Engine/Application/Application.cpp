#include "Engine/Application/Application.h"
#include "Engine/Layer/Layer.h"

#include "Engine/Core/Utilities.h"

#include <thread>

namespace Engine
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        Utilities::Log::Initialize();

        if (s_Instance)
        {
            TR_CORE_ERROR("Application already exists!");
            return;
        }

        s_Instance = this;
        TR_CORE_TRACE("Engine Core Started");

        m_LastFrameTime = std::chrono::steady_clock::now();

        m_Window = Window::Create();
    }

    Application::~Application()
    {
        TR_CORE_TRACE("Engine Core Shutdown");
        s_Instance = nullptr;
    }

    void Application::Run()
    {
        while (m_Running && !m_Window->ShouldClose())
        {
            m_Window->OnUpdate();

            auto a_Now = std::chrono::steady_clock::now();
            float l_DeltaTime = std::chrono::duration<float>(a_Now - m_LastFrameTime).count();
            m_LastFrameTime = a_Now;

            for (Layer* layer : m_LayerStack)
            {
                layer->OnUpdate(l_DeltaTime);
            }
            for (auto a_Index = m_LayerStack.rbegin(); a_Index != m_LayerStack.rend(); ++a_Index)
            {
                (*a_Index)->OnRender();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
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