#include <Forge/Editor/Panels/ConsolePanel.h>

#include <string>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>
#include <Forge/Editor/EditorTheme.h>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    void ConsolePanel::OnImGuiRender()
    {
        if (!m_Context.ShowConsole)
        {
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Begin(ICON_TR_CONSOLE "  Console", &m_Context.ShowConsole))
        {
            ImGui::PopStyleVar();
            RenderContents();
        }
        else
        {
            ImGui::PopStyleVar();
        }

        ImGui::End();
    }

    void ConsolePanel::RenderContents()
    {
        int l_Infos = 0;
        int l_Warnings = 0;
        int l_Errors = 0;

        Log::VisitMessages([&l_Infos, &l_Warnings, &l_Errors](const LogMessage& message)
        {
            if (message.Level == LogLevel::Warn)
            {
                ++l_Warnings;
            }
            else if (message.Level == LogLevel::Error || message.Level == LogLevel::Critical)
            {
                ++l_Errors;
            }
            else
            {
                ++l_Infos;
            }
        });

        // Unity's Console toolbar: Clear on the left, then the three severity toggles right-aligned.
        EditorTheme::BeginPanelToolBar("##ConsoleToolBar");

        if (EditorTheme::ToolBarButton(ICON_TR_CLEAR "  Clear", false, "Clear the console", true, ImGui::GetFontSize() * 5.0f))
        {
            Log::ClearMessages();
            m_SelectedMessage = -1;
        }

        ImGui::SameLine();
        if (EditorTheme::ToolBarButton(ICON_TR_COLLAPSE, false, nullptr, false, 0.0f))
        {
        }

        float l_CounterWidth = ImGui::GetFontSize() * 4.0f;
        ImGui::SameLine(ImGui::GetContentRegionMax().x - l_CounterWidth * 3.0f - 8.0f);

        if (EditorTheme::ToolBarButton((std::string(ICON_TR_INFO " ") + std::to_string(l_Infos)).c_str(), m_ShowInfo, "Log", true, l_CounterWidth, EditorColors::AppToolbarChecked))
        {
            m_ShowInfo = !m_ShowInfo;
        }

        ImGui::SameLine();
        if (EditorTheme::ToolBarButton((std::string(ICON_TR_WARNING " ") + std::to_string(l_Warnings)).c_str(), m_ShowWarnings, "Warning", true, l_CounterWidth, EditorColors::AppToolbarChecked))
        {
            m_ShowWarnings = !m_ShowWarnings;
        }

        ImGui::SameLine();
        if (EditorTheme::ToolBarButton((std::string(ICON_TR_ERROR " ") + std::to_string(l_Errors)).c_str(), m_ShowErrors, "Error", true, l_CounterWidth, EditorColors::AppToolbarChecked))
        {
            m_ShowErrors = !m_ShowErrors;
        }

        EditorTheme::EndPanelToolBar();

        // Unity splits the Console into a filtered list and a detail pane for the selected entry.
        float l_DetailHeight = ImGui::GetTextLineHeightWithSpacing() * 3.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 2.0f));
        ImGui::BeginChild("ConsoleScroll", ImVec2(0.0f, -l_DetailHeight), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_HorizontalScrollbar);

        int l_Index = 0;
        std::string l_SelectedText;

        Log::VisitMessages([this, &l_Index, &l_SelectedText](const LogMessage& message)
        {
            bool l_IsWarning = message.Level == LogLevel::Warn;
            bool l_IsError = message.Level == LogLevel::Error || message.Level == LogLevel::Critical;
            bool l_IsInfo = !l_IsWarning && !l_IsError;

            if ((l_IsInfo && !m_ShowInfo) || (l_IsWarning && !m_ShowWarnings) || (l_IsError && !m_ShowErrors))
            {
                return;
            }

            int l_Row = l_Index++;

            // Zebra striping, as in Unity's console list.
            if ((l_Row & 1) != 0)
            {
                ImVec2 l_Cursor = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(ImGui::GetWindowPos().x, l_Cursor.y), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, l_Cursor.y + ImGui::GetTextLineHeightWithSpacing()), EditorColors::AlternatedRow);
            }

            ImU32 l_Color = EditorColors::DefaultText;
            const char* l_Icon = ICON_TR_INFO;
            if (l_IsWarning)
            {
                l_Color = EditorColors::WarningText;
                l_Icon = ICON_TR_WARNING;
            }
            else if (l_IsError)
            {
                l_Color = EditorColors::ErrorText;
                l_Icon = ICON_TR_ERROR;
            }
            else if (message.Level == LogLevel::Trace)
            {
                l_Color = EditorColors::DisabledText;
            }

            ImGui::PushID(l_Row);
            ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::ToVec4(l_Color));
            if (ImGui::Selectable((std::string(l_Icon) + "  " + message.Text).c_str(), m_SelectedMessage == l_Row, ImGuiSelectableFlags_SpanAllColumns))
            {
                m_SelectedMessage = l_Row;
            }
            ImGui::PopStyleColor();
            ImGui::PopID();

            if (m_SelectedMessage == l_Row)
            {
                l_SelectedText = message.Text;
            }
        });

        if (m_Autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, EditorColors::ToVec4(EditorColors::DefaultBackground));
        ImGui::BeginChild("ConsoleDetail", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders);
        if (!l_SelectedText.empty())
        {
            ImGui::TextWrapped("%s", l_SelectedText.c_str());
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::PopStyleVar();
    }
}