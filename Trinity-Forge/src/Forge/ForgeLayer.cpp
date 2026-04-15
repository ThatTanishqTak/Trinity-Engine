#include "Forge/ForgeLayer.h"

#include "Trinity/Application/Application.h"
#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Renderer/Renderer.h"

#include <imgui.h>

ForgeLayer::ForgeLayer() : Trinity::Layer("ForgeLayer")
{
}

ForgeLayer::~ForgeLayer() = default;

void ForgeLayer::OnInitialize()
{
    m_SelectionContext.ActiveScene = &m_Scene;

    m_PanelManager.Initialize();

    m_ViewportPanel = m_PanelManager.RegisterPanel<Forge::ViewportPanel>("Viewport", &m_SelectionContext);
    m_HierarchyPanel = m_PanelManager.RegisterPanel<Forge::SceneHierarchyPanel>("Scene Hierarchy", &m_SelectionContext);
    m_InspectorPanel = m_PanelManager.RegisterPanel<Forge::InspectorPanel>("Inspector", &m_SelectionContext);
    m_ContentPanel = m_PanelManager.RegisterPanel<Forge::ContentBrowserPanel>("Content Browser");
    m_StatsPanel = m_PanelManager.RegisterPanel<Forge::RendererStatsPanel>("Renderer Stats");
    m_LogPanel = m_PanelManager.RegisterPanel<Forge::LogPanel>("Log");

    Trinity::ImGuiLayer::Get().SetMenuBarCallback([this]()
    {
        RenderMenuBar();
    });
}

void ForgeLayer::OnShutdown()
{
    m_PanelManager.Shutdown();
}

void ForgeLayer::OnUpdate(float deltaTime)
{
    m_PanelManager.UpdatePanels(deltaTime);
}

void ForgeLayer::OnRender()
{
}

void ForgeLayer::OnImGuiRender()
{
    m_PanelManager.RenderPanels();

    if (m_ShowAboutPopup)
    {
        ImGui::OpenPopup("About Trinity Engine");
        m_ShowAboutPopup = false;
    }

    ImVec2 l_Center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(l_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("About Trinity Engine", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Trinity Engine");
        ImGui::Separator();
        ImGui::Text("Vulkan-backed game engine with deferred rendering.");
        ImGui::Spacing();

        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    (void)e;
}

void ForgeLayer::RenderMenuBar()
{
    // File
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Scene", "Ctrl+N"))
        {

        }

        if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
        {

        }

        if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        {

        }

        if (ImGui::MenuItem("Save Scene As", "Ctrl+Shift+S"))
        {

        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit"))
        {
            Trinity::Application::Get().Close();
        }

        ImGui::EndMenu();
    }

    // Edit
    if (ImGui::BeginMenu("Edit"))
    {
        ImGui::BeginDisabled();
        ImGui::MenuItem("Undo", "Ctrl+Z");
        ImGui::MenuItem("Redo", "Ctrl+Y");
        ImGui::EndDisabled();

        ImGui::EndMenu();
    }

    // View
    if (ImGui::BeginMenu("View"))
    {
        m_PanelManager.RenderViewMenu();
        ImGui::EndMenu();
    }

    // Renderer
    if (ImGui::BeginMenu("Renderer"))
    {
        ImGui::Text("Backend: %s", Trinity::Renderer::GetBackend() == Trinity::RendererBackend::Vulkan ? "Vulkan" : "Unknown");

        ImGui::Separator();

        ImGui::BeginDisabled();
        ImGui::MenuItem("Show GeometryBuffer Albedo");
        ImGui::MenuItem("Show GeometryBuffer Normal");
        ImGui::MenuItem("Show GeometryBuffer MRA");
        ImGui::MenuItem("Show Shadow Map");
        ImGui::EndDisabled();

        ImGui::Separator();

        ImGui::MenuItem("Renderer Stats", nullptr, &m_StatsPanel->GetOpenState());

        ImGui::EndMenu();
    }

    // Help
    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("About Trinity Engine"))
        {
            m_ShowAboutPopup = true;
        }

        ImGui::EndMenu();
    }
}