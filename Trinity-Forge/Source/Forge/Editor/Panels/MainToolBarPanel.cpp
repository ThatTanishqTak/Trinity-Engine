#include <Forge/Editor/Panels/MainToolBarPanel.h>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>
#include <Forge/Editor/EditorTheme.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Scene/Scene.h>

namespace Trinity
{
    void MainToolBarPanel::OnImGuiRender()
    {
        ProcessRequests();

        float l_Height = EditorTheme::ToolBarHeight();
        float l_PadY = (l_Height - ImGui::GetFrameHeight()) * 0.5f - 1.0f;
        if (l_PadY < 0.0f)
        {
            l_PadY = 0.0f;
        }

        if (EditorTheme::BeginChromeBar("##ForgeMainToolBar", m_Context.ChromeTop, l_Height, EditorColors::AppToolbar, ImVec2(8.0f, l_PadY)))
        {
            float l_BarWidth = ImGui::GetWindowWidth();

            RenderLeftCluster();
            RenderPlayControls(l_BarWidth);
            RenderRightCluster(l_BarWidth);
        }

        EditorTheme::EndChromeBar();

        m_Context.ChromeTop += l_Height;
    }

    void MainToolBarPanel::TogglePlay()
    {
        if (m_Context.PlayMode)
        {
            m_Engine.StopScene();
            if (m_Engine.HasScene())
            {
                m_Context.PruneSelection(m_Engine.GetScene().GetRegistry());
            }

            m_Context.History.Clear();
            m_Context.PlayMode = false;
            m_Context.Paused = false;

            return;
        }

        if (m_Engine.StartScene())
        {
            // Undo does not cross the play boundary; entity handles recorded before play are stale after the snapshot restore.
            m_Context.History.Clear();
            m_Context.PlayMode = true;
            m_Context.Paused = false;
        }
    }

    void MainToolBarPanel::TogglePause()
    {
        if (!m_Context.PlayMode)
        {
            return;
        }

        m_Context.Paused = !m_Context.Paused;
        m_Engine.SetScenePaused(m_Context.Paused);
    }

    void MainToolBarPanel::ProcessRequests()
    {
        if (m_Context.PlayToggleRequested)
        {
            m_Context.PlayToggleRequested = false;
            TogglePlay();
        }

        if (m_Context.PauseToggleRequested)
        {
            m_Context.PauseToggleRequested = false;
            TogglePause();
        }

        if (m_Context.StepRequested)
        {
            m_Context.StepRequested = false;
            if (m_Context.PlayMode)
            {
                // Unity's Step implies Pause; the frame advances one fixed tick and stops again.
                if (!m_Context.Paused)
                {
                    m_Context.Paused = true;
                    m_Engine.SetScenePaused(true);
                }

                m_Engine.StepScene();
            }
        }
    }

    void MainToolBarPanel::RenderLeftCluster()
    {
        // Unity's left cluster is the services strip; Forge fills the same slot with its project actions.
        if (EditorTheme::ToolBarButton(ICON_TR_SAVE, false, "Save  (Ctrl+S)"))
        {
            m_Context.FileOp = PendingFileOp::Save;
        }

        ImGui::SameLine();
        if (EditorTheme::ToolBarButton(ICON_TR_REFRESH, false, "Refresh assets  (Ctrl+R)"))
        {
            m_Context.RefreshAssetsRequested = true;
        }

        EditorTheme::ToolBarSeparator();

        if (EditorTheme::ToolBarButton(ICON_TR_CLOUD "  Trinity Cloud  " ICON_TR_CHEVRON_DOWN, false, nullptr, false, ImGui::GetFontSize() * 9.5f))
        {
        }
    }

    void MainToolBarPanel::RenderPlayControls(float barWidth)
    {
        bool l_HasScene = m_Engine.HasScene();
        float l_ButtonWidth = ImGui::GetFrameHeight() + 10.0f;
        float l_GroupWidth = l_ButtonWidth * 3.0f;
        float l_GroupX = (barWidth - l_GroupWidth) * 0.5f;

        // Unity welds Play / Pause / Step into one segmented control at the exact centre of the toolbar.
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::SameLine(l_GroupX);

        if (EditorTheme::ToolBarButton(ICON_TR_PLAY, m_Context.PlayMode, m_Context.PlayMode ? "Stop  (Ctrl+P)" : "Play  (Ctrl+P)", l_HasScene, l_ButtonWidth, EditorColors::Highlight))
        {
            TogglePlay();
        }

        ImGui::SameLine();
        if (EditorTheme::ToolBarButton(ICON_TR_PAUSE, m_Context.Paused, "Pause  (Ctrl+Shift+P)", m_Context.PlayMode, l_ButtonWidth, EditorColors::Highlight))
        {
            TogglePause();
        }

        ImGui::SameLine();
        if (EditorTheme::ToolBarButton(ICON_TR_STEP, false, "Step  (Ctrl+Alt+P)", m_Context.PlayMode, l_ButtonWidth, EditorColors::Highlight))
        {
            m_Context.StepRequested = true;
        }

        ImGui::PopStyleVar();
    }

    void MainToolBarPanel::RenderRightCluster(float barWidth)
    {
        float l_IconWidth = ImGui::GetFrameHeight() + 6.0f;
        float l_SearchWidth = ImGui::GetFontSize() * 10.0f;
        float l_LayoutWidth = ImGui::GetFontSize() * 6.0f;
        float l_Total = l_IconWidth * 3.0f + l_SearchWidth + l_LayoutWidth + 16.0f;

        ImGui::SameLine(barWidth - l_Total);

        if (EditorTheme::ToolBarButton(ICON_TR_HISTORY, false, "Undo history"))
        {
            ImGui::OpenPopup("##ForgeUndoHistory");
        }

        if (ImGui::BeginPopup("##ForgeUndoHistory"))
        {
            std::string l_Undo = m_Context.History.CanUndo() ? ("Undo " + m_Context.History.UndoName()) : "Nothing to undo";
            if (ImGui::MenuItem(l_Undo.c_str(), "Ctrl+Z", false, m_Context.History.CanUndo()))
            {
                m_Context.History.Undo();
            }

            std::string l_Redo = m_Context.History.CanRedo() ? ("Redo " + m_Context.History.RedoName()) : "Nothing to redo";
            if (ImGui::MenuItem(l_Redo.c_str(), "Ctrl+Y", false, m_Context.History.CanRedo()))
            {
                m_Context.History.Redo();
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        EditorTheme::SearchField("##ForgeGlobalSearch", m_SearchBuffer, sizeof(m_SearchBuffer), l_SearchWidth);

        ImGui::SameLine();
        if (EditorTheme::ToolBarButton(ICON_TR_LAYOUT "  Layout  " ICON_TR_CHEVRON_DOWN, false, "Editor layout", true, l_LayoutWidth))
        {
            ImGui::OpenPopup("##ForgeLayoutMenu");
        }

        if (ImGui::BeginPopup("##ForgeLayoutMenu"))
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

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        EditorTheme::ToolBarButton(ICON_TR_HELP, false, "Trinity Manual", false);

        ImGui::SameLine();
        if (EditorTheme::ToolBarButton(ICON_TR_MORE, false, "Editor settings"))
        {
            ImGui::OpenPopup("##ForgeToolBarSettings");
        }

        if (ImGui::BeginPopup("##ForgeToolBarSettings"))
        {
            ImGui::MenuItem("Physics Debugger", nullptr, &m_Context.ShowPhysicsColliders);
            ImGui::MenuItem("Physics Settings", nullptr, &m_Context.ShowPhysicsSettings);
            ImGui::MenuItem("Render Graph", nullptr, &m_Context.ShowRenderGraph);
            ImGui::Separator();
            ImGui::MenuItem("Preferences...", nullptr, false, false);
            ImGui::EndPopup();
        }
    }
}