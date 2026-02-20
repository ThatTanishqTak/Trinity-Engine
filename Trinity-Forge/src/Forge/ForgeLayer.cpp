#include "ForgeLayer.h"

#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Geometry/Geometry.h"
#include "Trinity/Application/Application.h"
#include "Trinity/Platform/Window.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Events/ApplicationEvent.h"
#include "Trinity/Events/KeyEvent.h"
#include "Trinity/Events/MouseEvent.h"

ForgeLayer::ForgeLayer() : Trinity::Layer("ForgeLayer")
{

}

ForgeLayer::~ForgeLayer() = default;

void ForgeLayer::OnInitialize()
{
    auto& a_Window = Trinity::Application::Get().GetWindow();
    m_EditorCamera.SetViewportSize(static_cast<float>(a_Window.GetWidth()), static_cast<float>(a_Window.GetHeight()));
}

void ForgeLayer::OnShutdown()
{
    // Safety: never leave the cursor clipped/hidden if the layer shuts down mid-look.
    if (m_IsLooking)
    {
        auto& a_Window = Trinity::Application::Get().GetWindow();
        a_Window.SetCursorLocked(false);
        a_Window.SetCursorVisible(true);
        m_IsLooking = false;
    }
}

void ForgeLayer::OnUpdate(float deltaTime)
{
    // Allow camera input while the scene viewport is active OR while we're in RMB look mode.
    m_EditorCamera.SetInputEnabled(m_CanControlCamera || m_IsLooking);
    m_EditorCamera.OnUpdate(deltaTime);
}

void ForgeLayer::OnRender()
{
    Trinity::RenderCommand::DrawMesh(Trinity::Geometry::PrimitiveType::Cube, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, m_EditorCamera.GetViewMatrix(), m_EditorCamera.GetProjectionMatrix());
}

void ForgeLayer::OnImGuiRender()
{
    ImGuiWindowFlags l_WindowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* l_MainViewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(l_MainViewport->Pos);
    ImGui::SetNextWindowSize(l_MainViewport->Size);
    ImGui::SetNextWindowViewport(l_MainViewport->ID);

    l_WindowFlags |= ImGuiWindowFlags_NoTitleBar;
    l_WindowFlags |= ImGuiWindowFlags_NoCollapse;
    l_WindowFlags |= ImGuiWindowFlags_NoResize;
    l_WindowFlags |= ImGuiWindowFlags_NoMove;
    l_WindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    l_WindowFlags |= ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("ForgeDockspace", nullptr, l_WindowFlags);
    ImGui::PopStyleVar(2);

    const ImGuiID l_DockspaceID = ImGui::GetID("ForgeDockspaceID");
    ImGui::DockSpace(l_DockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::Begin("Scene");

    const bool l_IsSceneViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    const bool l_IsSceneViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    m_CanControlCamera = l_IsSceneViewportFocused && l_IsSceneViewportHovered;

    auto& a_Window = Trinity::Application::Get().GetWindow();
    if (m_CanControlCamera && !m_IsLooking && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        m_IsLooking = true;
        a_Window.SetCursorVisible(false);
        a_Window.SetCursorLocked(true);
    }

    if (m_IsLooking && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        m_IsLooking = false;
        a_Window.SetCursorLocked(false);
        a_Window.SetCursorVisible(true);
    }

    const ImVec2 l_ViewportPanelSize = ImGui::GetContentRegionAvail();
    const uint32_t l_SceneViewportWidth = static_cast<uint32_t>(l_ViewportPanelSize.x > 0.0f ? l_ViewportPanelSize.x : 0.0f);
    const uint32_t l_SceneViewportHeight = static_cast<uint32_t>(l_ViewportPanelSize.y > 0.0f ? l_ViewportPanelSize.y : 0.0f);

    Trinity::RenderCommand::SetSceneViewportSize(l_SceneViewportWidth, l_SceneViewportHeight);

    const ImVec2 l_SceneViewportSize = ImVec2(static_cast<float>(l_SceneViewportWidth), static_cast<float>(l_SceneViewportHeight));
    if (l_SceneViewportSize.x != m_SceneViewportSize.x || l_SceneViewportSize.y != m_SceneViewportSize.y)
    {
        m_SceneViewportSize = l_SceneViewportSize;

        if (l_SceneViewportWidth > 0 && l_SceneViewportHeight > 0)
        {
            m_EditorCamera.SetViewportSize(static_cast<float>(l_SceneViewportWidth), static_cast<float>(l_SceneViewportHeight));
        }
    }

    ImTextureID l_SceneTexture = reinterpret_cast<ImTextureID>(Trinity::RenderCommand::GetSceneViewportHandle());
    if (l_SceneTexture != (ImTextureID)nullptr && l_ViewportPanelSize.x > 0.0f && l_ViewportPanelSize.y > 0.0f)
    {
        ImGui::Image(l_SceneTexture, l_ViewportPanelSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
    }

    ImGui::End(); // Scene
    ImGui::End(); // Dockspace
}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    Trinity::EventDispatcher l_Dispatcher(e);

    // If the window loses focus mid-look (alt-tab, click another window), stop looking and restore cursor.
    l_Dispatcher.Dispatch<Trinity::WindowLostFocusEvent>([this](Trinity::WindowLostFocusEvent&)
    {
        if (m_IsLooking)
        {
            auto& a_Window = Trinity::Application::Get().GetWindow();
            a_Window.SetCursorLocked(false);
            a_Window.SetCursorVisible(true);
            m_IsLooking = false;
        }

        return false;
    });

    l_Dispatcher.Dispatch<Trinity::KeyPressedEvent>([this](Trinity::KeyPressedEvent& keyPressedEvent)
    {
        if (keyPressedEvent.GetKeyCode() == Trinity::Code::KeyCode::TR_KEY_ESCAPE)
        {
            // ESC exits RMB-look first.
            if (m_IsLooking)
            {
                auto& a_Window = Trinity::Application::Get().GetWindow();
                a_Window.SetCursorLocked(false);
                a_Window.SetCursorVisible(true);
                m_IsLooking = false;
            
                return true;
            }

            Trinity::Application::Close();

            return true;
        }
        return false;
    });

    // Only feed raw mouse deltas to the camera while in RMB look mode.
    l_Dispatcher.Dispatch<Trinity::MouseRawDeltaEvent>([this](Trinity::MouseRawDeltaEvent& rawDeltaEvent)
    {
        if (!m_IsLooking)
        {
            return false;
        }

        m_EditorCamera.OnEvent(rawDeltaEvent);

        return true;
    });

    if (!e.Handled && (m_CanControlCamera || m_IsLooking))
    {
        // Avoid double-processing raw delta (handled above).
        if (e.GetEventType() != Trinity::EventType::MouseRawDelta)
        {
            m_EditorCamera.OnEvent(e);
        }
    }
}