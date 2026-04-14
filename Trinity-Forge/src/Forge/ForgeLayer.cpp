#include "Forge/ForgeLayer.h"

#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Events/Event.h"

#include <imgui.h>

ForgeLayer::ForgeLayer() : Trinity::Layer("ForgeLayer")
{
}

ForgeLayer::~ForgeLayer() = default;

void ForgeLayer::OnInitialize()
{
    m_PanelManager.Initialize();

    m_ViewportPanel  = m_PanelManager.RegisterPanel<Forge::ViewportPanel>("Viewport");
    m_HierarchyPanel = m_PanelManager.RegisterPanel<Forge::SceneHierarchyPanel>("Scene Hierarchy", &m_SelectionContext);
    m_InspectorPanel = m_PanelManager.RegisterPanel<Forge::InspectorPanel>("Inspector", &m_SelectionContext);
    m_ContentPanel   = m_PanelManager.RegisterPanel<Forge::ContentBrowserPanel>("Content Browser");
    m_StatsPanel     = m_PanelManager.RegisterPanel<Forge::RendererStatsPanel>("Renderer Stats");
    m_LogPanel       = m_PanelManager.RegisterPanel<Forge::LogPanel>("Log");

    Trinity::ImGuiLayer::Get().SetMenuBarCallback([this]() { RenderMenuBar(); });
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
}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    (void)e;
}

void ForgeLayer::RenderMenuBar()
{
    if (ImGui::BeginMenu("View"))
    {
        m_PanelManager.RenderViewMenu();
        ImGui::EndMenu();
    }
}
