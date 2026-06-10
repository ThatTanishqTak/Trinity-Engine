#include <Forge/Editor/Panels/MenuBarPanel.h>

#include <string>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>

#include <Trinity/Core/Application.h>
#include <Trinity/Core/Engine.h>
#include <Trinity/Platform/Window.h>
#include <Trinity/Renderer/RHI/GraphicsDevice.h>
#include <Trinity/ImGui/IImGuiRenderBackend.h>
#include <Trinity/Assets/AssetDatabase.h>

namespace Trinity
{
    static constexpr float k_LogoAspect = 1.5f;

    void MenuBarPanel::OnImGuiRender()
    {
        HandleShortcuts();

        Window& l_Window = Application::Get().GetWindow();
        if (l_Window.HasCustomTitleBar())
        {
            RenderTitleBar(l_Window);
        }
        else
        {
            RenderMenuBar();
        }
    }

    void MenuBarPanel::EnsureLogo()
    {
        if (m_LogoTried)
        {
            return;
        }

        m_LogoTried = true;

        if (!m_Engine.HasAssetDatabase() || !m_Engine.HasDevice())
        {
            return;
        }

        UUID l_LogoAsset = m_Engine.GetAssetDatabase().GetAssetByPath("Assets/Logo.png");
        if (static_cast<uint64_t>(l_LogoAsset) == 0)
        {
            return;
        }

        TextureHandle l_Logo = m_Engine.GetAssetDatabase().ResolveTexture(l_LogoAsset);
        m_LogoTexture = m_Engine.GetDevice().GetImGuiBackend().RegisterTexture(l_Logo);
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
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && m_Context.SelectedEntity != entt::null)
        {
            m_Context.Action = PendingAction::Delete;
            m_Context.ActionTarget = m_Context.SelectedEntity;
        }
    }

    void MenuBarPanel::RenderMenus()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, ImGui::GetStyle().WindowPadding.y));

        bool l_HasSelection = m_Context.SelectedEntity != entt::null;

        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem(ICON_FA_FILE "  New Scene", "Ctrl+N", false, false);
            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN "  Open Scene", "Ctrl+O"))
            {
                m_Context.FileOp = PendingFileOp::Load;
            }
            
            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  Save Scene", "Ctrl+S"))
            {
                m_Context.FileOp = PendingFileOp::Save;
            }
            
            ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S", false, false);
            ImGui::MenuItem("Save All", nullptr, false, false);
            ImGui::Separator();
            ImGui::MenuItem("New Project...", nullptr, false, false);
            ImGui::MenuItem("Open Project...", nullptr, false, false);
            if (ImGui::BeginMenu("Recent Projects"))
            {
                ImGui::MenuItem("(none)", nullptr, false, false);
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_XMARK "  Exit", "Alt+F4"))
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
            ImGui::MenuItem("Cut", "Ctrl+X", false, false);
            ImGui::MenuItem("Copy", "Ctrl+C", false, false);
            ImGui::MenuItem("Paste", "Ctrl+V", false, false);
            if (ImGui::MenuItem(ICON_FA_COPY "  Duplicate", "Ctrl+D", false, l_HasSelection))
            {
                m_Context.Action = PendingAction::Duplicate;
                m_Context.ActionTarget = m_Context.SelectedEntity;
            }
            
            if (ImGui::MenuItem(ICON_FA_TRASH "  Delete", "Del", false, l_HasSelection))
            {
                m_Context.Action = PendingAction::Delete;
                m_Context.ActionTarget = m_Context.SelectedEntity;
            }
            ImGui::Separator();
            ImGui::MenuItem(ICON_FA_GEAR "  Editor Preferences...", nullptr, false, false);
            ImGui::MenuItem("Project Settings...", nullptr, false, false);
            ImGui::MenuItem("Plugins", nullptr, false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            ImGui::MenuItem("Viewport", nullptr, false, false);
            ImGui::MenuItem(ICON_FA_SITEMAP "  Outliner", nullptr, false, false);
            ImGui::MenuItem(ICON_FA_SLIDERS "  Details", nullptr, false, false);

            if (ImGui::MenuItem(ICON_FA_FOLDER "  Content Browser", nullptr, m_Context.ShowContentDrawer))
            {
                m_Context.ShowContentDrawer = !m_Context.ShowContentDrawer;
                if (m_Context.ShowContentDrawer)
                {
                    m_Context.ShowConsoleDrawer = false;
                }
            }
            
            if (ImGui::MenuItem(ICON_FA_TERMINAL "  Console", nullptr, m_Context.ShowConsoleDrawer))
            {
                m_Context.ShowConsoleDrawer = !m_Context.ShowConsoleDrawer;
                if (m_Context.ShowConsoleDrawer)
                {
                    m_Context.ShowContentDrawer = false;
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE "  Reset Layout"))
            {
                m_Context.ResetLayout = true;
            }

            Window& l_Window = Application::Get().GetWindow();
            if (ImGui::MenuItem(ICON_FA_EXPAND "  Enable Fullscreen", "F11", l_Window.IsMaximized()))
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

        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("Build Tool", nullptr, false, false);
            ImGui::MenuItem("Asset Cooker", nullptr, false, false);
            ImGui::MenuItem("Shader Compiler", nullptr, false, false);
            ImGui::Separator();
            ImGui::MenuItem("Material Editor", nullptr, false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Build"))
        {
            ImGui::MenuItem("Build All", nullptr, false, false);
            ImGui::MenuItem("Build Lighting", nullptr, false, false);
            ImGui::Separator();
            ImGui::MenuItem("Build Profiles...", nullptr, false, false);
            ImGui::MenuItem("Package Project", nullptr, false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Select"))
        {
            ImGui::MenuItem("Select All", "Ctrl+A", false, false);
            if (ImGui::MenuItem("Select None", "Esc", false, l_HasSelection))
            {
                m_Context.SelectedEntity = entt::null;
            }

            ImGui::MenuItem("Invert Selection", "Ctrl+I", false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Actor"))
        {
            if (ImGui::MenuItem(ICON_FA_PLUS "  Create Empty"))
            {
                m_Context.Action = PendingAction::Create;
            }
            
            if (ImGui::MenuItem(ICON_FA_COPY "  Duplicate", "Ctrl+D", false, l_HasSelection))
            {
                m_Context.Action = PendingAction::Duplicate;
                m_Context.ActionTarget = m_Context.SelectedEntity;
            }

            if (ImGui::MenuItem(ICON_FA_TRASH "  Delete", "Del", false, l_HasSelection))
            {
                m_Context.Action = PendingAction::Delete;
                m_Context.ActionTarget = m_Context.SelectedEntity;
            }
            
            ImGui::Separator();
            ImGui::MenuItem("Attach To...", nullptr, false, false);
            ImGui::MenuItem("Detach", nullptr, false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem("Documentation", nullptr, false, false);
            ImGui::MenuItem("About Forge", nullptr, false, false);
            ImGui::EndMenu();
        }

        ImGui::PopStyleVar();
    }

    void MenuBarPanel::RenderMenuBar()
    {
        if (!ImGui::BeginMainMenuBar())
        {
            return;
        }

        RenderMenus();

        std::string l_Title = m_Context.SceneName + (m_Context.History.IsDirty() ? " *" : "");
        float l_TitleWidth = ImGui::CalcTextSize(l_Title.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - l_TitleWidth - 12.0f);
        ImGui::TextUnformatted(l_Title.c_str());

        ImGui::EndMainMenuBar();
    }

    void MenuBarPanel::RenderTitleBar(Window& window)
    {
        ImGuiStyle& l_Style = ImGui::GetStyle();

        float l_BarPadding = ImGui::GetFontSize() * 0.35f;
        float l_Height = ImGui::GetFontSize() + l_BarPadding * 2.0f + 2.0f;

        ImGuiViewport* l_Viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(l_Viewport->Pos);
        ImGui::SetNextWindowSize(ImVec2(l_Viewport->Size.x, l_Height));
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        ImGuiWindowFlags l_Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(l_Style.FramePadding.x, l_BarPadding));
        ImGui::Begin("##ForgeTitleBar", nullptr, l_Flags);

        if (ImGui::BeginMenuBar())
        {
            EnsureLogo();

            if (m_LogoTexture != 0)
            {
                float l_LogoH = ImGui::GetFrameHeight() - 6.0f;
                ImGui::Image((ImTextureID)m_LogoTexture, ImVec2(l_LogoH * k_LogoAspect, l_LogoH));
                ImGui::SameLine();
            }

            RenderMenus();

            float l_MenusEndX = ImGui::GetCursorPosX();
            float l_BarWidth = ImGui::GetWindowWidth();
            float l_ButtonWidth = ImGui::GetFrameHeight();
            float l_ButtonsWidth = l_ButtonWidth * 3.0f + l_Style.ItemSpacing.x * 2.0f;
            float l_ButtonsX = l_BarWidth - l_ButtonsWidth - l_Style.ItemSpacing.x;

            std::string l_Title = m_Context.SceneName + (m_Context.History.IsDirty() ? " *" : "") + "   -   Forge";
            float l_TitleWidth = ImGui::CalcTextSize(l_Title.c_str()).x;
            float l_TitleX = (l_BarWidth - l_TitleWidth) * 0.5f;
            if (l_TitleX + l_TitleWidth > l_ButtonsX - l_Style.ItemSpacing.x)
            {
                l_TitleX = l_ButtonsX - l_Style.ItemSpacing.x - l_TitleWidth;
            }

            if (l_TitleX < l_MenusEndX + l_Style.ItemSpacing.x)
            {
                l_TitleX = l_MenusEndX + l_Style.ItemSpacing.x;
            }

            ImGui::SameLine(l_TitleX);
            ImGui::TextDisabled("%s", l_Title.c_str());

            ImGui::SameLine(l_ButtonsX);
            if (ImGui::Button(ICON_FA_MINUS, ImVec2(l_ButtonWidth, 0.0f)))
            {
                window.Minimize();
            }

            ImGui::SameLine();
            if (ImGui::Button(window.IsMaximized() ? ICON_FA_COMPRESS : ICON_FA_EXPAND, ImVec2(l_ButtonWidth, 0.0f)))
            {
                if (window.IsMaximized())
                {
                    window.Restore();
                }
                else
                {
                    window.Maximize();
                }
            }

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_XMARK, ImVec2(l_ButtonWidth, 0.0f)))
            {
                window.RequestClose();
            }

            float l_WinX = ImGui::GetWindowPos().x;
            float l_WinY = ImGui::GetWindowPos().y;
            int l_HitX = static_cast<int>((l_WinX + l_MenusEndX) - l_Viewport->Pos.x);
            int l_HitY = static_cast<int>(l_WinY - l_Viewport->Pos.y);
            int l_HitWidth = static_cast<int>(l_ButtonsX - l_MenusEndX);
            int l_HitHeight = static_cast<int>(l_Height);
            window.SetTitleBarHitRegion(l_HitX, l_HitY, l_HitWidth > 0 ? l_HitWidth : 0, l_HitHeight);

            ImGui::EndMenuBar();
        }

        ImGui::End();
        ImGui::PopStyleVar(4);

        m_Context.ChromeTop += l_Height;
    }
}