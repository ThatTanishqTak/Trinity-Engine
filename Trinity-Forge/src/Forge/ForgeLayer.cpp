#include "ForgeLayer.h"

#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Geometry/Geometry.h"
#include "Trinity/Application/Application.h"
#include "Trinity/Platform/Window.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Events/ApplicationEvent.h"
#include "Trinity/Events/KeyEvent.h"

#include <imgui.h>

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

}

void ForgeLayer::OnUpdate(float deltaTime)
{
    m_EditorCamera.OnUpdate(deltaTime);
}

void ForgeLayer::OnRender()
{
    Trinity::RenderCommand::DrawMesh(Trinity::Geometry::PrimitiveType::Cube, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, m_EditorCamera.GetViewMatrix(),
        m_EditorCamera.GetProjectionMatrix());
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

    ImGui::ShowDemoWindow();

    ImGui::End();
}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    Trinity::EventDispatcher l_Dispatcher(e);
    l_Dispatcher.Dispatch<Trinity::KeyPressedEvent>([](Trinity::KeyPressedEvent& keyPressedEvent)
    {
        if (keyPressedEvent.GetKeyCode() == Trinity::Code::KeyCode::TR_KEY_ESCAPE)
        {
            Trinity::Application::Close();

            return true;
        }

        return false;
    });

    if (e.GetEventType() == Trinity::EventType::WindowResize)
    {
        auto& a_ResizeEvent = static_cast<Trinity::WindowResizeEvent&>(e);
        m_EditorCamera.SetViewportSize(static_cast<float>(a_ResizeEvent.GetWidth()), static_cast<float>(a_ResizeEvent.GetHeight()));
    }

    if (!e.Handled)
    {
        m_EditorCamera.OnEvent(e);
    }
}