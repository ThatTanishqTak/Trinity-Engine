#pragma once

#include "Forge/Panels/PanelManager.h"
#include "Forge/Panels/ViewportPanel.h"
#include "Forge/Panels/SceneHierarchyPanel.h"
#include "Forge/Panels/InspectorPanel.h"
#include "Forge/Panels/ContentBrowserPanel.h"
#include "Forge/Panels/RendererStatsPanel.h"
#include "Forge/Panels/LogPanel.h"

#include "Trinity/Layer/Layer.h"

#include <entt/entt.hpp>

namespace Trinity
{
    class Event;
    class Scene;
}

struct SelectionContext
{
    entt::entity SelectedEntity = entt::null;
    Trinity::Scene* ActiveScene = nullptr;  // Gap: Scene system (Phase 18)
};

class ForgeLayer : public Trinity::Layer
{
public:
    ForgeLayer();
    ~ForgeLayer() override;

    void OnInitialize() override;
    void OnShutdown() override;

    void OnUpdate(float deltaTime) override;
    void OnRender() override;

    void OnImGuiRender() override;

    void OnEvent(Trinity::Event& e) override;

private:
    void RenderMenuBar();

    Forge::PanelManager m_PanelManager;
    SelectionContext m_SelectionContext;

    Forge::ViewportPanel*        m_ViewportPanel  = nullptr;
    Forge::SceneHierarchyPanel*  m_HierarchyPanel = nullptr;
    Forge::InspectorPanel*       m_InspectorPanel = nullptr;
    Forge::ContentBrowserPanel*  m_ContentPanel   = nullptr;
    Forge::RendererStatsPanel*   m_StatsPanel     = nullptr;
    Forge::LogPanel*             m_LogPanel       = nullptr;
};
