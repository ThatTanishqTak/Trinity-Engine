#pragma once

#include <functional>
#include <memory>

#include <entt/entt.hpp>

#include <Trinity/Core/Application.h>
#include <Trinity/Core/Timestep.h>

#include <Forge/Editor/EditorContext.h>

namespace Trinity
{
    class Scene;

    class MenuBarPanel;
    class MainToolBarPanel;
    class StatusBarPanel;
    class ViewportPanel;
    class HierarchyPanel;
    class InspectorPanel;
    class ConsolePanel;
    class ContentBrowserPanel;

    class ForgeApplication : public Application
    {
    public:
        ForgeApplication(const ApplicationSpecification& specification);
        ~ForgeApplication() override;

        void OnInitialize() override;
        void OnUpdate(Timestep timestep) override;
        void OnImGuiRender() override;
        void OnEvent(Event& event, bool handled) override;
        void OnShutdown() override;

    private:
        void RenderDockspace();
        void BuildDefaultLayout(unsigned int dockspaceID);
        void RenderDrawer(const char* id, const char* title, bool& show, bool& openPrev, const std::function<void()>& body);
        void ProcessPendingAction();
        void ProcessDeferredComponentOp();
        void ProcessPendingFileOp();
        bool IsAncestorOf(Scene& scene, entt::entity ancestor, entt::entity node);

        EditorContext m_Context;
        std::unique_ptr<MenuBarPanel> m_MenuBarPanel;
        std::unique_ptr<MainToolBarPanel> m_MainToolBarPanel;
        std::unique_ptr<StatusBarPanel> m_StatusBarPanel;
        std::unique_ptr<ViewportPanel> m_ViewportPanel;
        std::unique_ptr<HierarchyPanel> m_HierarchyPanel;
        std::unique_ptr<InspectorPanel> m_InspectorPanel;
        std::unique_ptr<ConsolePanel> m_ConsolePanel;
        std::unique_ptr<ContentBrowserPanel> m_ContentBrowserPanel;

        bool m_ContentDrawerOpenPrev = false;
        bool m_ConsoleDrawerOpenPrev = false;
        float m_DrawerHeight = 0.0f;
    };
}