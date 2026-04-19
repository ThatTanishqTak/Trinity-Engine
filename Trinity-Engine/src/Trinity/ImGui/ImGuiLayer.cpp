#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/ImGui/ImGuiTheme.h"
#include "Trinity/ImGui/Platform/Vulkan/ImGuiVulkanBackend.h"

#include "Trinity/Application/Application.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Platform/Window/Window.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#include "Trinity/Utilities/Log.h"

#include <cstdlib>

#include <imgui.h>

namespace Trinity
{
    ImGuiLayer* ImGuiLayer::s_Instance = nullptr;

    ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
    {
        s_Instance = this;
    }

    ImGuiLayer::~ImGuiLayer()
    {
        s_Instance = nullptr;
    }

    void ImGuiLayer::OnInitialize()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& l_IO = ImGui::GetIO();
        l_IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGuiTheme::SetDarkTheme();
        ImGuiTheme::LoadFonts();

        auto& a_API = static_cast<VulkanRendererAPI&>(Renderer::GetAPI());
        SDL_Window* l_Window = static_cast<SDL_Window*>(Application::Get().GetWindow().GetNativeHandle().Window);

        m_Implementation = std::make_unique<Implementation>();
        m_Implementation->Initialize(l_Window, a_API);

        Application::Get().GetWindow().SetPlatformEventCallback([this](const void* event)
        {
            m_Implementation->ProcessPlatformEvent(event);
        });

        TR_CORE_INFO("ImGui layer initialized");
    }

    void ImGuiLayer::OnShutdown()
    {
        Renderer::WaitIdle();
        m_Implementation->Shutdown();
        m_Implementation.reset();
        ImGui::DestroyContext();

        TR_CORE_INFO("ImGui layer shut down");
    }

    void ImGuiLayer::OnEvent(Event& e)
    {
        ImGuiIO& l_IO = ImGui::GetIO();

        if (e.IsInCategory(EventCategoryMouse))
        {
            e.Handled |= l_IO.WantCaptureMouse;
        }

        if (e.IsInCategory(EventCategoryKeyboard))
        {
            e.Handled |= l_IO.WantCaptureKeyboard;
        }
    }

    void ImGuiLayer::Begin()
    {
        m_Implementation->NewFrame();
        ImGui::NewFrame();
        PushDockspace();
    }

    void ImGuiLayer::End()
    {
        ImGui::Render();
        m_Implementation->Render();
    }

    void ImGuiLayer::SetMenuBarCallback(std::function<void()> callback)
    {
        m_MenuBarCallback = std::move(callback);
    }

    void ImGuiLayer::SetTitleBarCallback(std::function<void()> callback)
    {
        m_TitleBarCallback = std::move(callback);
    }

    uint64_t ImGuiLayer::RegisterTexture(const std::shared_ptr<Texture>& texture)
    {
        return m_Implementation->RegisterTexture(texture->GetOpaqueHandle());
    }

    void ImGuiLayer::UnregisterTexture(uint64_t textureID)
    {
        m_Implementation->UnregisterTexture(textureID);
    }

    ImGuiLayer& ImGuiLayer::Get()
    {
        if (s_Instance == nullptr)
        {
            TR_CORE_CRITICAL("ImGuiLayer instance not available.");
            std::abort();
        }

        return *s_Instance;
    }

    void ImGuiLayer::PushDockspace()
    {
        const ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
        const float l_TitleBarHeight = ImGui::GetTextLineHeight() + 14.0f;

        ImGui::SetNextWindowPos(l_Viewport->WorkPos);
        ImGui::SetNextWindowSize(ImVec2(l_Viewport->WorkSize.x, l_TitleBarHeight));
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        constexpr ImGuiWindowFlags l_TitleBarFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
            | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 1.00f));

        ImGui::Begin("##TrinityTitleBar", nullptr, l_TitleBarFlags);
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();

        if (m_TitleBarCallback)
        {
            m_TitleBarCallback();
        }

        ImGui::End();

        const ImVec2 l_DockPos = ImVec2(l_Viewport->WorkPos.x, l_Viewport->WorkPos.y + l_TitleBarHeight);
        const ImVec2 l_DockSize = ImVec2(l_Viewport->WorkSize.x, l_Viewport->WorkSize.y - l_TitleBarHeight);

        ImGui::SetNextWindowPos(l_DockPos);
        ImGui::SetNextWindowSize(l_DockSize);
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        ImGuiWindowFlags l_Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("##TrinityDockspace", nullptr, l_Flags);
        ImGui::PopStyleVar(3);

        const ImGuiID l_DockspaceID = ImGui::GetID("TrinityDockspace");
        ImGui::DockSpace(l_DockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        if (ImGui::BeginMenuBar())
        {
            if (m_MenuBarCallback)
            {
                m_MenuBarCallback();
            }

            ImGui::EndMenuBar();
        }

        ImGui::End();
    }
}
