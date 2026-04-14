#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/ImGui/ImGuiTheme.h"
#include "Trinity/ImGui/Platform/Vulkan/ImGuiVulkanBackend.h"

#include "Trinity/Application/Application.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Platform/Window/Window.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#include "Trinity/Utilities/Log.h"

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

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGuiTheme::SetDarkTheme();

        auto& l_API    = static_cast<VulkanRendererAPI&>(Renderer::GetAPI());
        SDL_Window* l_Window = static_cast<SDL_Window*>(Application::Get().GetWindow().GetNativeHandle().Window);

        m_Impl = std::make_unique<Impl>();
        m_Impl->Initialize(l_Window, l_API);

        Application::Get().GetWindow().SetPlatformEventCallback([this](const void* event)
        {
            m_Impl->ProcessPlatformEvent(event);
        });

        TR_CORE_INFO("ImGui layer initialized.");
    }

    void ImGuiLayer::OnShutdown()
    {
        Renderer::WaitIdle();
        m_Impl->Shutdown();
        m_Impl.reset();
        ImGui::DestroyContext();

        TR_CORE_INFO("ImGui layer shut down.");
    }

    void ImGuiLayer::OnEvent(Event& e)
    {
        ImGuiIO& io = ImGui::GetIO();

        if (e.IsInCategory(EventCategoryMouse))
            e.Handled |= io.WantCaptureMouse;

        if (e.IsInCategory(EventCategoryKeyboard))
            e.Handled |= io.WantCaptureKeyboard;
    }

    void ImGuiLayer::Begin()
    {
        m_Impl->NewFrame();
        ImGui::NewFrame();
        PushDockspace();
    }

    void ImGuiLayer::End()
    {
        ImGui::Render();
        m_Impl->Render();
    }

    void ImGuiLayer::SetMenuBarCallback(std::function<void()> callback)
    {
        m_MenuBarCallback = std::move(callback);
    }

    uint64_t ImGuiLayer::RegisterTexture(const std::shared_ptr<Texture>& texture)
    {
        return m_Impl->RegisterTexture(texture->GetOpaqueHandle());
    }

    void ImGuiLayer::UnregisterTexture(uint64_t textureID)
    {
        m_Impl->UnregisterTexture(textureID);
    }

    ImGuiLayer& ImGuiLayer::Get()
    {
        return *s_Instance;
    }

    void ImGuiLayer::PushDockspace()
    {
        const ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(l_Viewport->WorkPos);
        ImGui::SetNextWindowSize(l_Viewport->WorkSize);
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        ImGuiWindowFlags l_Flags =
            ImGuiWindowFlags_NoTitleBar      |
            ImGuiWindowFlags_NoCollapse      |
            ImGuiWindowFlags_NoResize        |
            ImGuiWindowFlags_NoMove          |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus      |
            ImGuiWindowFlags_NoBackground    |
            ImGuiWindowFlags_MenuBar;

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
                m_MenuBarCallback();

            ImGui::EndMenuBar();
        }

        ImGui::End();
    }
}
