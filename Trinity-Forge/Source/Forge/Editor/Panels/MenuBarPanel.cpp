#include <Forge/Editor/Panels/MenuBarPanel.h>

#include <string>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>

namespace Trinity
{
    void MenuBarPanel::OnImGuiRender()
    {
        HandleShortcuts();
        RenderMenuBar();
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

    void MenuBarPanel::RenderMenuBar()
    {
        if (!ImGui::BeginMainMenuBar())
        {
            return;
        }

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                m_Context.FileOp = PendingFileOp::Save;
            }

            if (ImGui::MenuItem("Open"))
            {
                m_Context.FileOp = PendingFileOp::Load;
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

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Entity"))
        {
            if (ImGui::MenuItem("Create Empty"))
            {
                m_Context.Action = PendingAction::Create;
            }

            bool l_HasSelection = m_Context.SelectedEntity != entt::null;
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

            ImGui::EndMenu();
        }

        std::string l_Title = m_Context.SceneName + (m_Context.History.IsDirty() ? " *" : "");
        float l_TitleWidth = ImGui::CalcTextSize(l_Title.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - l_TitleWidth - 12.0f);
        ImGui::TextUnformatted(l_Title.c_str());

        ImGui::EndMainMenuBar();
    }
}