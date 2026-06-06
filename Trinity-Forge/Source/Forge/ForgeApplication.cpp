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
#include <ImGuizmo.h>

#include <Forge/Editor/Panels/MenuBarPanel.h>
#include <Forge/Editor/Panels/ViewportPanel.h>
#include <Forge/Editor/Panels/HierarchyPanel.h>
#include <Forge/Editor/Panels/InspectorPanel.h>
#include <Forge/Editor/Panels/ConsolePanel.h>
#include <Forge/Editor/Panels/ContentBrowserPanel.h>
#include <Forge/Editor/Commands/EntityCommands.h>

#include <memory>

namespace Trinity
{
    ForgeApplication::ForgeApplication(const ApplicationSpecification& specification) : Application(specification)
    {

    }

    ForgeApplication::~ForgeApplication() = default;

    void ForgeApplication::OnInitialize()
    {
        GetEngine().InitializeImGui();
        m_Context.ScenePath = GetEngine().GetPlatform().GetFileSystem().Resolve(BaseDirectory::UserData, "Scenes/Demo.tscene");

        m_MenuBarPanel = std::make_unique<MenuBarPanel>(m_Context, GetEngine());
        m_ViewportPanel = std::make_unique<ViewportPanel>(m_Context, GetEngine());
        m_HierarchyPanel = std::make_unique<HierarchyPanel>(m_Context, GetEngine());
        m_InspectorPanel = std::make_unique<InspectorPanel>(m_Context, GetEngine());
        m_ConsolePanel = std::make_unique<ConsolePanel>(m_Context, GetEngine());
        m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>(m_Context, GetEngine());

        TR_INFO("Forge initialized");
    }

    void ForgeApplication::OnUpdate(Timestep)
    {

    }

    void ForgeApplication::OnImGuiRender()
    {
        ImGuizmo::BeginFrame();

        m_MenuBarPanel->OnImGuiRender();
        RenderDockspace();
        m_ViewportPanel->OnImGuiRender();
        m_HierarchyPanel->OnImGuiRender();
        ProcessPendingAction();
        m_ConsolePanel->OnImGuiRender();
        m_ContentBrowserPanel->OnImGuiRender();
        m_InspectorPanel->OnImGuiRender();
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
            TR_INFO("Key pressed: {}", key.GetKeyCode());
            return false;
        });
    }

    void ForgeApplication::OnShutdown()
    {
        TR_INFO("Forge shutting down");
    }

    void ForgeApplication::RenderDockspace()
    {
        ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(l_Viewport->WorkPos);
        ImGui::SetNextWindowSize(l_Viewport->WorkSize);
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        ImGuiWindowFlags l_HostFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("TrinityDockspaceHost", nullptr, l_HostFlags);
        ImGui::PopStyleVar(3);

        ImGuiID l_DockspaceID = ImGui::GetID("TrinityDockspace");
        ImGui::DockSpace(l_DockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();
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

        if (m_Context.Action == PendingAction::Create)
        {
            std::unique_ptr<CreateEntityCommand> l_Command = std::make_unique<CreateEntityCommand>(l_Scene, "Entity");
            CreateEntityCommand* l_Raw = l_Command.get();
            m_Context.History.Execute(std::move(l_Command));

            Entity l_Created = FindEntityByUUID(l_Scene, l_Raw->GetResultUUID());
            m_Context.SelectedEntity = l_Created.IsValid() ? l_Created.GetHandle() : entt::null;
        }
        else if (m_Context.Action == PendingAction::Duplicate)
        {
            if (m_Context.ActionTarget != entt::null && l_Scene.GetRegistry().valid(m_Context.ActionTarget))
            {
                std::unique_ptr<DuplicateEntityCommand> l_Command = std::make_unique<DuplicateEntityCommand>(l_Scene, GetEngine().GetAssetDatabase(), m_Context.ActionTarget);
                DuplicateEntityCommand* l_Raw = l_Command.get();
                m_Context.History.Execute(std::move(l_Command));

                Entity l_Duplicate = FindEntityByUUID(l_Scene, l_Raw->GetResultUUID());
                m_Context.SelectedEntity = l_Duplicate.IsValid() ? l_Duplicate.GetHandle() : entt::null;
            }
        }
        else if (m_Context.Action == PendingAction::Delete)
        {
            if (m_Context.ActionTarget != entt::null && l_Scene.GetRegistry().valid(m_Context.ActionTarget))
            {
                m_Context.History.Execute(std::make_unique<DeleteEntityCommand>(l_Scene, GetEngine().GetAssetDatabase(), m_Context.ActionTarget));

                if (m_Context.SelectedEntity == m_Context.ActionTarget)
                {
                    m_Context.SelectedEntity = entt::null;
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
            m_Context.SelectedEntity = entt::null;
            SceneSerializer::Deserialize(l_Scene, GetEngine().GetAssetDatabase(), m_Context.ScenePath);
            m_Context.History.Clear();
        }

        m_Context.FileOp = PendingFileOp::None;
    }

    Application* CreateApplication(CommandLineArgs args)
    {
        ApplicationSpecification l_Specification;
        l_Specification.InternalName = "Trinity-Forge";
        l_Specification.Window.Title = "Forge";
        l_Specification.Window.Width = 1920;
        l_Specification.Window.Height = 1080;
        l_Specification.Arguments = args;

        return new ForgeApplication(l_Specification);
    }
}