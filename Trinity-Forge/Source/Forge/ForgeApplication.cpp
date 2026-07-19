#include <Forge/ForgeApplication.h>

#include <Trinity/Core/EntryPoint.h>
#include <Trinity/Core/Engine.h>
#include <Trinity/Core/Log.h>
#include <Trinity/Core/Event.h>
#include <Trinity/Platform/Events/KeyEvent.h>
#include <Trinity/Platform/IPlatform.h>
#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/HierarchyComponent.h>
#include <Trinity/Serialization/SceneSerializer.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

#include <Forge/Editor/EditorTheme.h>
#include <Forge/Editor/Panels/MenuBarPanel.h>
#include <Forge/Editor/Panels/MainToolBarPanel.h>
#include <Forge/Editor/Panels/StatusBarPanel.h>
#include <Forge/Editor/Panels/ViewportPanel.h>
#include <Forge/Editor/Panels/HierarchyPanel.h>
#include <Forge/Editor/Panels/InspectorPanel.h>
#include <Forge/Editor/Panels/ConsolePanel.h>
#include <Forge/Editor/Panels/ContentBrowserPanel.h>
#include <Forge/Editor/Panels/RenderGraphPanel.h>
#include <Forge/Editor/Commands/EntityCommands.h>
#include <Forge/Editor/Commands/CompositeCommand.h>

#include <memory>

namespace Trinity
{
    ForgeApplication::ForgeApplication(const ApplicationSpecification& specification) : Application(specification)
    {

    }

    ForgeApplication::~ForgeApplication() = default;

    void ForgeApplication::OnInitialize()
    {
        ("INITIALIZING FORGE");

        GetEngine().InitializeImGui();
        EditorTheme::Apply(GetEngine().GetPlatform().GetFileSystem());

        m_Context.ScenePath = GetEngine().GetPlatform().GetFileSystem().Resolve(BaseDirectory::UserData, "Scenes/Demo.tscene");

        m_MenuBarPanel = std::make_unique<MenuBarPanel>(m_Context, GetEngine());
        m_MainToolBarPanel = std::make_unique<MainToolBarPanel>(m_Context, GetEngine());
        m_StatusBarPanel = std::make_unique<StatusBarPanel>(m_Context, GetEngine());
        m_ViewportPanel = std::make_unique<ViewportPanel>(m_Context, GetEngine());
        m_HierarchyPanel = std::make_unique<HierarchyPanel>(m_Context, GetEngine());
        m_InspectorPanel = std::make_unique<InspectorPanel>(m_Context, GetEngine());
        m_ConsolePanel = std::make_unique<ConsolePanel>(m_Context, GetEngine());
        m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>(m_Context, GetEngine());
        m_RenderGraphPanel = std::make_unique<RenderGraphPanel>(m_Context, GetEngine());
        
        ("FORGE INITIALIZED");
    }

    void ForgeApplication::OnUpdate(Timestep)
    {

    }

    void ForgeApplication::OnImGuiRender()
    {
        ImGuizmo::BeginFrame();

        m_Context.ChromeTop = 0.0f;
        m_Context.ChromeBottom = 0.0f;
        m_Context.DrawerToggled = false;

        m_MenuBarPanel->OnImGuiRender();
        m_MainToolBarPanel->OnImGuiRender();
        m_StatusBarPanel->OnImGuiRender();
        RenderDockspace();
        m_ViewportPanel->OnImGuiRender();
        m_HierarchyPanel->OnImGuiRender();
        ProcessPendingAction();
        m_InspectorPanel->OnImGuiRender();
        m_RenderGraphPanel->OnImGuiRender();

        RenderDrawer("##ForgeContentDrawer", "Content Browser", m_Context.ShowContentDrawer, m_ContentDrawerOpenPrev, [this]() { m_ContentBrowserPanel->RenderContents(); });
        RenderDrawer("##ForgeConsoleDrawer", "Console", m_Context.ShowConsoleDrawer, m_ConsoleDrawerOpenPrev, [this]() { m_ConsolePanel->RenderContents(); });

        ProcessDeferredComponentOp();
        ProcessPendingFileOp();
    }

    void ForgeApplication::OnEvent(Event& event, bool handled)
    {
        if (handled)
        {
            return;
        }

        EventDispatcher l_Dispatcher(event);
        l_Dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent& key)
        {
            //("Key pressed: {}", key.GetKeyCode());
            return false;
        });
    }

    void ForgeApplication::OnShutdown()
    {
        ("SHUTTING DOWN FORGE");

        m_Context.History.Clear();

        ("FORGE SHUTDOWN COMPLETE");
    }

    void ForgeApplication::RenderDockspace()
    {
        ImGuiViewport* l_Viewport = ImGui::GetMainViewport();

        ImVec2 l_Pos = ImVec2(l_Viewport->Pos.x, l_Viewport->Pos.y + m_Context.ChromeTop);
        ImVec2 l_Size = ImVec2(l_Viewport->Size.x, l_Viewport->Size.y - m_Context.ChromeTop - m_Context.ChromeBottom);

        ImGui::SetNextWindowPos(l_Pos);
        ImGui::SetNextWindowSize(l_Size);
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        ImGuiWindowFlags l_HostFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("TrinityDockspaceHost", nullptr, l_HostFlags);
        ImGui::PopStyleVar(3);

        ImGuiID l_DockspaceID = ImGui::GetID("TrinityDockspace");
        ImGui::DockSpace(l_DockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        if (m_Context.ResetLayout || ImGui::DockBuilderGetNode(l_DockspaceID) == nullptr)
        {
            BuildDefaultLayout(l_DockspaceID);
            m_Context.ResetLayout = false;
        }

        ImGui::End();
    }

    void ForgeApplication::BuildDefaultLayout(unsigned int dockspaceID)
    {
        ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
        ImVec2 l_Size = ImVec2(l_Viewport->Size.x, l_Viewport->Size.y - m_Context.ChromeTop - m_Context.ChromeBottom);

        ImGui::DockBuilderRemoveNode(dockspaceID);
        ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceID, l_Size);

        ImGuiID l_Main = dockspaceID;
        ImGuiID l_Right = ImGui::DockBuilderSplitNode(l_Main, ImGuiDir_Right, 0.22f, nullptr, &l_Main);
        ImGuiID l_RightBottom = ImGui::DockBuilderSplitNode(l_Right, ImGuiDir_Down, 0.55f, nullptr, &l_Right);

        ImGui::DockBuilderDockWindow("Viewport", l_Main);
        ImGui::DockBuilderDockWindow("Outliner", l_Right);
        ImGui::DockBuilderDockWindow("Details", l_RightBottom);

        ImGui::DockBuilderFinish(dockspaceID);
    }

    void ForgeApplication::RenderDrawer(const char* id, const char* title, bool& show, bool& openPrev, const std::function<void()>& body)
    {
        if (!show)
        {
            openPrev = false;

            return;
        }

        ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
        float l_Bottom = l_Viewport->Pos.y + l_Viewport->Size.y - m_Context.ChromeBottom;
        float l_MinHeight = l_Viewport->Size.y * 0.12f;
        float l_MaxHeight = l_Viewport->Size.y * 0.85f;

        if (m_DrawerHeight <= 0.0f)
        {
            m_DrawerHeight = l_Viewport->Size.y * 0.40f;
        }

        if (m_DrawerHeight < l_MinHeight)
        {
            m_DrawerHeight = l_MinHeight;
        }

        if (m_DrawerHeight > l_MaxHeight)
        {
            m_DrawerHeight = l_MaxHeight;
        }

        ImGui::SetNextWindowPos(ImVec2(l_Viewport->Pos.x, l_Bottom - m_DrawerHeight), ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(l_Viewport->Size.x, l_MinHeight), ImVec2(l_Viewport->Size.x, l_MaxHeight));
        ImGui::SetNextWindowSize(ImVec2(l_Viewport->Size.x, m_DrawerHeight), ImGuiCond_Once);
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        if (!openPrev)
        {
            ImGui::SetNextWindowFocus();
        }

        ImGuiWindowFlags l_Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin(id, nullptr, l_Flags);

        ImVec2 l_DrawerMin = ImGui::GetWindowPos();
        ImVec2 l_DrawerSize = ImGui::GetWindowSize();
        m_DrawerHeight = l_DrawerSize.y;

        ImGui::TextUnformatted(title);
        ImGui::Separator();

        bool l_PopupActive = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);

        body();

        ImGui::End();

        bool l_Close = false;
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !l_PopupActive && !ImGui::GetIO().WantTextInput)
        {
            l_Close = true;
        }
        else if (openPrev && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsDragDropActive() && !l_PopupActive)
        {
            ImVec2 l_Mouse = ImGui::GetIO().MousePos;
            bool l_InDrawer = l_Mouse.x >= l_DrawerMin.x && l_Mouse.x <= l_DrawerMin.x + l_DrawerSize.x && l_Mouse.y >= l_DrawerMin.y && l_Mouse.y <= l_DrawerMin.y + l_DrawerSize.y;
            bool l_InStatusBar = l_Mouse.y >= l_Bottom;
            if (!l_InDrawer && !l_InStatusBar)
            {
                l_Close = true;
            }
        }

        if (l_Close)
        {
            show = false;
        }

        openPrev = show;
    }

    bool ForgeApplication::IsAncestorOf(Scene& scene, entt::entity ancestor, entt::entity node)
    {
        entt::registry& l_Registry = scene.GetRegistry();
        entt::entity l_Current = node;
        while (l_Current != entt::null)
        {
            if (l_Current == ancestor)
            {
                return true;
            }

            const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(l_Current);
            l_Current = l_Hierarchy != nullptr ? l_Hierarchy->Parent : entt::null;
        }

        return false;
    }

    void ForgeApplication::ProcessPendingAction()
    {
        if (m_Context.Action == PendingAction::None || !GetEngine().HasScene())
        {
            m_Context.Action = PendingAction::None;
            m_Context.ActionTarget = entt::null;

            return;
        }

        Scene& l_Scene = GetEngine().GetScene();

        // Actions triggered on a member of a multi-selection apply to the whole selection, reduced to top-most entities so selected parents don't double-process selected children.
        auto l_CollectActionTargets = [this, &l_Scene](entt::entity primary) -> std::vector<entt::entity>
            {
                std::vector<entt::entity> l_Targets;

                if (m_Context.Selection.size() <= 1 || !m_Context.IsSelected(primary))
                {
                    l_Targets.push_back(primary);

                    return l_Targets;
                }

                for (entt::entity it_Entity : m_Context.Selection)
                {
                    if (!l_Scene.GetRegistry().valid(it_Entity))
                    {
                        continue;
                    }

                    bool l_HasSelectedAncestor = false;
                    for (entt::entity it_Other : m_Context.Selection)
                    {
                        if (it_Other != it_Entity && l_Scene.GetRegistry().valid(it_Other) && IsAncestorOf(l_Scene, it_Other, it_Entity))
                        {
                            l_HasSelectedAncestor = true;

                            break;
                        }
                    }

                    if (!l_HasSelectedAncestor)
                    {
                        l_Targets.push_back(it_Entity);
                    }
                }

                return l_Targets;
            };

        if (m_Context.Action == PendingAction::Create)
        {
            std::unique_ptr<CreateEntityCommand> l_Command = std::make_unique<CreateEntityCommand>(l_Scene, "Entity");
            CreateEntityCommand* l_Raw = l_Command.get();
            m_Context.History.Execute(std::move(l_Command));

            Entity l_Created = FindEntityByUUID(l_Scene, l_Raw->GetResultUUID());
            m_Context.Select(l_Created.IsValid() ? l_Created.GetHandle() : entt::null);
        }
        else if (m_Context.Action == PendingAction::Duplicate)
        {
            if (m_Context.ActionTarget != entt::null && l_Scene.GetRegistry().valid(m_Context.ActionTarget))
            {
                std::vector<entt::entity> l_Targets = l_CollectActionTargets(m_Context.ActionTarget);

                std::unique_ptr<CompositeCommand> l_Composite = std::make_unique<CompositeCommand>(l_Targets.size() > 1 ? "Duplicate Entities" : "Duplicate Entity");
                std::vector<DuplicateEntityCommand*> l_Commands;
                for (entt::entity it_Target : l_Targets)
                {
                    std::unique_ptr<DuplicateEntityCommand> l_Command = std::make_unique<DuplicateEntityCommand>(l_Scene, GetEngine().GetAssetDatabase(), it_Target);
                    l_Commands.push_back(l_Command.get());
                    l_Composite->Add(std::move(l_Command));
                }

                m_Context.History.Execute(std::move(l_Composite));

                m_Context.ClearSelection();
                for (DuplicateEntityCommand* it_Command : l_Commands)
                {
                    Entity l_Duplicate = FindEntityByUUID(l_Scene, it_Command->GetResultUUID());
                    if (l_Duplicate.IsValid())
                    {
                        m_Context.AddToSelection(l_Duplicate.GetHandle());
                    }
                }
            }
        }
        else if (m_Context.Action == PendingAction::Delete)
        {
            if (m_Context.ActionTarget != entt::null && l_Scene.GetRegistry().valid(m_Context.ActionTarget))
            {
                std::vector<entt::entity> l_Targets = l_CollectActionTargets(m_Context.ActionTarget);

                std::unique_ptr<CompositeCommand> l_Composite = std::make_unique<CompositeCommand>(l_Targets.size() > 1 ? "Delete Entities" : "Delete Entity");
                for (entt::entity it_Target : l_Targets)
                {
                    l_Composite->Add(std::make_unique<DeleteEntityCommand>(l_Scene, GetEngine().GetAssetDatabase(), it_Target));
                }

                m_Context.History.Execute(std::move(l_Composite));

                for (entt::entity it_Target : l_Targets)
                {
                    m_Context.Deselect(it_Target);
                }
            }
        }
        else if (m_Context.Action == PendingAction::Reparent)
        {
            if (m_Context.ActionTarget != entt::null && l_Scene.GetRegistry().valid(m_Context.ActionTarget))
            {
                entt::entity l_NewParent = m_Context.ActionParent;
                bool l_Valid = l_NewParent == entt::null || l_Scene.GetRegistry().valid(l_NewParent);
                if (l_Valid && IsAncestorOf(l_Scene, m_Context.ActionTarget, l_NewParent))
                {
                    l_Valid = false;
                }

                if (l_Valid)
                {
                    uint64_t l_ChildUUID = static_cast<uint64_t>(Entity(m_Context.ActionTarget, &l_Scene).GetUUID());
                    uint64_t l_ParentUUID = l_NewParent != entt::null ? static_cast<uint64_t>(Entity(l_NewParent, &l_Scene).GetUUID()) : 0;
                    m_Context.History.Execute(std::make_unique<SetParentCommand>(l_Scene, l_ChildUUID, l_ParentUUID));
                }
            }
        }

        m_Context.Action = PendingAction::None;
        m_Context.ActionTarget = entt::null;
        m_Context.ActionParent = entt::null;
    }

    void ForgeApplication::ProcessDeferredComponentOp()
    {
        if (m_Context.ComponentOp)
        {
            std::function<void()> l_Op = std::move(m_Context.ComponentOp);
            m_Context.ComponentOp = nullptr;
            l_Op();
        }
    }

    void ForgeApplication::ProcessPendingFileOp()
    {
        if (m_Context.FileOp == PendingFileOp::None || !GetEngine().HasScene())
        {
            m_Context.FileOp = PendingFileOp::None;

            return;
        }

        Scene& l_Scene = GetEngine().GetScene();

        if (m_Context.FileOp == PendingFileOp::Save)
        {
            if (SceneSerializer::Serialize(l_Scene, m_Context.ScenePath, m_Context.SceneName))
            {
                m_Context.History.MarkSaved();
            }
        }
        else if (m_Context.FileOp == PendingFileOp::Load)
        {
            l_Scene.GetRegistry().clear();
            m_Context.ClearSelection();
            SceneSerializer::Deserialize(l_Scene, GetEngine().GetAssetDatabase(), m_Context.ScenePath);
            m_Context.History.Clear();
        }

        m_Context.FileOp = PendingFileOp::None;
    }

    Application* CreateApplication(CommandLineArgs args)
    {
        ("CREATING APPLICATION");

        ApplicationSpecification l_Specification;
        l_Specification.InternalName = "Trinity-Forge";
        l_Specification.Window.Title = "Forge";
        l_Specification.Window.Width = 1920;
        l_Specification.Window.Height = 1080;
        l_Specification.Window.CustomTitleBar = true;
        l_Specification.Arguments = args;

        ("Application internal name: {}, Application window title: {}", l_Specification.InternalName, l_Specification.Window.Title);
        ("Application resolution: {}x{}, Custom title bar: {}", l_Specification.Window.Width, l_Specification.Window.Height, l_Specification.Window.CustomTitleBar);
        ("Application argument count: {}", l_Specification.Arguments.Count);

        for (int i = 0; i < l_Specification.Arguments.Count; ++i)
        {
            ("Argument [{}]: {}", i, l_Specification.Arguments[i]);
        }

        ("APPLICATION CREATED");

        return new ForgeApplication(l_Specification);
    }
}