#include <Forge/Editor/Panels/MenuBarPanel.h>

#include <string>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>
#include <Forge/Editor/EditorTheme.h>

#include <Trinity/Core/Application.h>
#include <Trinity/Core/Engine.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/NameComponent.h>
#include <Trinity/Platform/Window.h>

namespace Trinity
{
    void MenuBarPanel::OnImGuiRender()
    {
        HandleShortcuts();

        float l_Height = EditorTheme::MenuBarHeight();

        if (EditorTheme::BeginChromeBar("##ForgeMenuBar", m_Context.ChromeTop, l_Height, EditorColors::AppToolbar, ImVec2(0.0f, 0.0f), ImGuiWindowFlags_MenuBar))
        {
            if (ImGui::BeginMenuBar())
            {
                RenderMenus();

                ImGui::EndMenuBar();
            }
        }

        EditorTheme::EndChromeBar();

        RenderAboutModal();

        m_Context.ChromeTop += l_Height;
    }

    void MenuBarPanel::SelectAll()
    {
        if (!m_Engine.HasScene())
        {
            return;
        }

        m_Context.ClearSelection();

        auto l_View = m_Engine.GetScene().GetRegistry().view<NameComponent>();
        for (entt::entity it_Entity : l_View)
        {
            m_Context.Selection.push_back(it_Entity);
        }

        m_Context.SelectedEntity = m_Context.Selection.empty() ? entt::null : m_Context.Selection.back();
    }

    void MenuBarPanel::HandleShortcuts()
    {
        ImGuiIO& l_IO = ImGui::GetIO();
        if (l_IO.WantTextInput)
        {
            return;
        }

        if (l_IO.KeyCtrl)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
            {
                if (l_IO.KeyShift)
                {
                    m_Context.History.Redo();
                }
                else
                {
                    m_Context.History.Undo();
                }
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
            {
                m_Context.History.Redo();
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_D, false) && m_Context.SelectedEntity != entt::null)
            {
                m_Context.Action = PendingAction::Duplicate;
                m_Context.ActionTarget = m_Context.SelectedEntity;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_S, false))
            {
                m_Context.FileOp = PendingFileOp::Save;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_O, false))
            {
                m_Context.FileOp = PendingFileOp::Load;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_R, false))
            {
                m_Context.RefreshAssetsRequested = true;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_A, false))
            {
                SelectAll();
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_N, false) && l_IO.KeyShift)
            {
                m_Context.Action = PendingAction::Create;
            }
            // Unity: Ctrl+P plays, Ctrl+Shift+P pauses, Ctrl+Alt+P steps one frame.
            else if (ImGui::IsKeyPressed(ImGuiKey_P, false))
            {
                if (l_IO.KeyShift)
                {
                    m_Context.PauseToggleRequested = true;
                }
                else if (l_IO.KeyAlt)
                {
                    m_Context.StepRequested = true;
                }
                else
                {
                    m_Context.PlayToggleRequested = true;
                }
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && m_Context.SelectedEntity != entt::null)
        {
            m_Context.Action = PendingAction::Delete;
            m_Context.ActionTarget = m_Context.SelectedEntity;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_F11, false) && Application::Get().HasWindow())
        {
            Window& l_Window = Application::Get().GetWindow();
            if (l_Window.IsMaximized())
            {
                l_Window.Restore();
            }
            else
            {
                l_Window.Maximize();
            }
        }
    }

    void MenuBarPanel::RenderMenus()
    {
        // Unity's menus are compact with a generous gap between the label and its shortcut.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));

        bool l_HasSelection = m_Context.SelectedEntity != entt::null;
        bool l_HasScene = m_Engine.HasScene();

        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("New Scene", "Ctrl+N", false, false);
            if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
            {
                m_Context.FileOp = PendingFileOp::Load;
            }

            if (ImGui::BeginMenu("Open Recent Scene"))
            {
                ImGui::MenuItem("(none)", nullptr, false, false);
                ImGui::EndMenu();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                m_Context.FileOp = PendingFileOp::Save;
            }

            ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, false);
            ImGui::Separator();
            ImGui::MenuItem("New Project...", nullptr, false, false);
            ImGui::MenuItem("Open Project...", nullptr, false, false);
            ImGui::MenuItem("Save Project", nullptr, false, false);
            ImGui::Separator();
            ImGui::MenuItem("Build Profiles...", nullptr, false, false);
            ImGui::MenuItem("Build And Run", "Ctrl+B", false, false);
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
            {
                Application::Get().GetWindow().RequestClose();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            std::string l_UndoLabel = m_Context.History.CanUndo() ? ("Undo " + m_Context.History.UndoName()) : "Undo";
            if (ImGui::MenuItem(l_UndoLabel.c_str(), "Ctrl+Z", false, m_Context.History.CanUndo()))
            {
                m_Context.History.Undo();
            }

            std::string l_RedoLabel = m_Context.History.CanRedo() ? ("Redo " + m_Context.History.RedoName()) : "Redo";
            if (ImGui::MenuItem(l_RedoLabel.c_str(), "Ctrl+Y", false, m_Context.History.CanRedo()))
            {
                m_Context.History.Redo();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A", false, l_HasScene))
            {
                SelectAll();
            }

            if (ImGui::MenuItem("Deselect All", "Shift+D", false, l_HasSelection))
            {
                m_Context.ClearSelection();
            }

            ImGui::MenuItem("Invert Selection", "Ctrl+I", false, false);
            ImGui::Separator();
            ImGui::MenuItem("Cut", "Ctrl+X", false, false);
            ImGui::MenuItem("Copy", "Ctrl+C", false, false);
            ImGui::MenuItem("Paste", "Ctrl+V", false, false);
            if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, l_HasSelection))
            {
                m_Context.Action = PendingAction::Duplicate;
                m_Context.ActionTarget = m_Context.SelectedEntity;
            }

            if (ImGui::MenuItem("Delete", "Del", false, l_HasSelection))
            {
                m_Context.Action = PendingAction::Delete;
                m_Context.ActionTarget = m_Context.SelectedEntity;
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Play", "Ctrl+P", m_Context.PlayMode, l_HasScene))
            {
                m_Context.PlayToggleRequested = true;
            }

            if (ImGui::MenuItem("Pause", "Ctrl+Shift+P", m_Context.Paused, m_Context.PlayMode))
            {
                m_Context.PauseToggleRequested = true;
            }

            if (ImGui::MenuItem("Step", "Ctrl+Alt+P", false, m_Context.PlayMode))
            {
                m_Context.StepRequested = true;
            }

            ImGui::Separator();
            ImGui::MenuItem("Project Settings...", nullptr, false, false);
            ImGui::MenuItem("Preferences...", nullptr, false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Assets"))
        {
            if (ImGui::BeginMenu("Create"))
            {
                ImGui::MenuItem("Folder", nullptr, false, false);
                ImGui::MenuItem("Material", nullptr, false, false);
                ImGui::MenuItem("Scene", nullptr, false, false);
                ImGui::EndMenu();
            }

            ImGui::Separator();
            ImGui::MenuItem("Show in Explorer", nullptr, false, false);
            ImGui::MenuItem("Import New Asset...", nullptr, false, false);
            ImGui::Separator();
            if (ImGui::MenuItem("Refresh", "Ctrl+R"))
            {
                m_Context.RefreshAssetsRequested = true;
            }

            ImGui::MenuItem("Reimport All", nullptr, false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject"))
        {
            if (ImGui::MenuItem("Create Empty", "Ctrl+Shift+N"))
            {
                m_Context.Action = PendingAction::Create;
            }

            ImGui::MenuItem("Create Empty Child", "Alt+Shift+N", false, false);
            ImGui::Separator();
            if (ImGui::BeginMenu("3D Object"))
            {
                ImGui::MenuItem("Cube", nullptr, false, false);
                ImGui::MenuItem("Plane", nullptr, false, false);
                ImGui::MenuItem("Quad", nullptr, false, false);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Light"))
            {
                ImGui::MenuItem("Directional Light", nullptr, false, false);
                ImGui::MenuItem("Point Light", nullptr, false, false);
                ImGui::EndMenu();
            }

            ImGui::MenuItem("Camera", nullptr, false, false);
            ImGui::Separator();
            if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, l_HasSelection))
            {
                m_Context.Action = PendingAction::Duplicate;
                m_Context.ActionTarget = m_Context.SelectedEntity;
            }

            if (ImGui::MenuItem("Delete", "Del", false, l_HasSelection))
            {
                m_Context.Action = PendingAction::Delete;
                m_Context.ActionTarget = m_Context.SelectedEntity;
            }

            if (ImGui::MenuItem("Unparent", nullptr, false, l_HasSelection))
            {
                m_Context.Action = PendingAction::Reparent;
                m_Context.ActionTarget = m_Context.SelectedEntity;
                m_Context.ActionParent = entt::null;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Component"))
        {
            if (ImGui::MenuItem("Add...", "Ctrl+Shift+A", false, l_HasSelection))
            {
                m_Context.OpenAddComponent = true;
            }

            ImGui::Separator();
            ImGui::MenuItem("Mesh", nullptr, false, false);
            ImGui::MenuItem("Physics 2D", nullptr, false, false);
            ImGui::MenuItem("Rendering", nullptr, false, false);
            ImGui::MenuItem("Audio", nullptr, false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::BeginMenu("Layouts"))
            {
                if (ImGui::MenuItem("Default"))
                {
                    m_Context.ResetLayout = true;
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Reset All Layouts"))
                {
                    m_Context.ResetLayout = true;
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();
            if (ImGui::BeginMenu("General"))
            {
                ImGui::MenuItem(ICON_TR_SCENE "  Scene", nullptr, &m_Context.ShowScene);
                ImGui::MenuItem(ICON_TR_GAME "  Game", nullptr, false, false);
                ImGui::MenuItem(ICON_TR_INSPECTOR "  Inspector", nullptr, &m_Context.ShowInspector);
                ImGui::MenuItem(ICON_TR_HIERARCHY "  Hierarchy", nullptr, &m_Context.ShowHierarchy);
                ImGui::MenuItem(ICON_TR_PROJECT "  Project", nullptr, &m_Context.ShowProject);
                ImGui::MenuItem(ICON_TR_CONSOLE "  Console", nullptr, &m_Context.ShowConsole);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Rendering"))
            {
                ImGui::MenuItem("Render Graph", nullptr, &m_Context.ShowRenderGraph);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Analysis"))
            {
                ImGui::MenuItem("Physics Debugger", nullptr, &m_Context.ShowPhysicsColliders);
                ImGui::MenuItem("Physics Settings", nullptr, &m_Context.ShowPhysicsSettings);
                ImGui::MenuItem("Log Physics Events", nullptr, &m_Context.LogPhysicsEvents);
                ImGui::EndMenu();
            }

            ImGui::Separator();
            Window& l_Window = Application::Get().GetWindow();
            if (ImGui::MenuItem("Maximize Window", "F11", l_Window.IsMaximized()))
            {
                if (l_Window.IsMaximized())
                {
                    l_Window.Restore();
                }
                else
                {
                    l_Window.Maximize();
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About Trinity Forge"))
            {
                m_OpenAbout = true;
            }

            ImGui::Separator();
            ImGui::MenuItem("Trinity Manual", nullptr, false, false);
            ImGui::MenuItem("Scripting Reference", nullptr, false, false);
            ImGui::MenuItem("Check for Updates", nullptr, false, false);
            ImGui::EndMenu();
        }

        ImGui::PopStyleVar(2);
    }

    void MenuBarPanel::RenderAboutModal()
    {
        if (m_OpenAbout)
        {
            ImGui::OpenPopup("About Trinity Forge");
            m_OpenAbout = false;
        }

        ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(l_Viewport->Pos.x + l_Viewport->Size.x * 0.5f, l_Viewport->Pos.y + l_Viewport->Size.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("About Trinity Forge", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::TextUnformatted("Trinity Forge");
            ImGui::TextDisabled("Trinity Engine editor");
            ImGui::Separator();
            ImGui::TextDisabled("Renderer: Vulkan");
            ImGui::TextDisabled("Platform: SDL3");
            ImGui::Spacing();

            if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}