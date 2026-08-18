#include <Forge/Editor/Panels/StatusBarPanel.h>

#include <string>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>
#include <Forge/Editor/EditorTheme.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Core/Log.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/IDComponent.h>

namespace Trinity
{
    void StatusBarPanel::OnImGuiRender()
    {
        float l_Height = EditorTheme::StatusBarHeight();

        ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
        float l_Y = l_Viewport->Size.y - m_Context.ChromeBottom - l_Height;

        // Unity's status bar mirrors the newest console entry on the left and keeps the
        // editor-wide toggles on the right.
        std::string l_LastMessage;
        LogLevel l_LastLevel = LogLevel::Info;
        int l_Warnings = 0;
        int l_Errors = 0;

        Log::VisitMessages([&l_LastMessage, &l_LastLevel, &l_Warnings, &l_Errors](const LogMessage& message)
        {
            if (message.Level == LogLevel::Warn)
            {
                ++l_Warnings;
            }
            else if (message.Level == LogLevel::Error || message.Level == LogLevel::Critical)
            {
                ++l_Errors;
            }

            if (message.Level != LogLevel::Trace)
            {
                l_LastMessage = message.Text;
                l_LastLevel = message.Level;
            }
        });

        if (EditorTheme::BeginChromeBar("##ForgeStatusBar", l_Y, l_Height, EditorColors::AppToolbar, ImVec2(8.0f, 1.0f)))
        {
            const char* l_Icon = ICON_TR_INFO;
            ImU32 l_Color = EditorColors::DisabledText;
            if (l_LastLevel == LogLevel::Warn)
            {
                l_Icon = ICON_TR_WARNING;
                l_Color = EditorColors::WarningText;
            }
            else if (l_LastLevel == LogLevel::Error || l_LastLevel == LogLevel::Critical)
            {
                l_Icon = ICON_TR_ERROR;
                l_Color = EditorColors::ErrorText;
            }

            ImGui::AlignTextToFramePadding();
            if (!l_LastMessage.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::ToVec4(l_Color));
                ImGui::TextUnformatted(l_Icon);
                ImGui::PopStyleColor();

                ImGui::SameLine(0.0f, 6.0f);
                ImGui::TextUnformatted(l_LastMessage.c_str());

                if (ImGui::IsItemClicked())
                {
                    m_Context.ShowConsole = true;
                }
            }

            int l_EntityCount = 0;
            if (m_Engine.HasScene())
            {
                l_EntityCount = static_cast<int>(m_Engine.GetScene().GetRegistry().view<IDComponent>().size());
            }

            std::string l_Summary = std::to_string(l_EntityCount) + " GameObjects";
            if (m_Context.History.IsDirty())
            {
                l_Summary += "    Unsaved changes";
            }

            float l_IconWidth = ImGui::GetFrameHeight() + 6.0f;
            float l_SummaryWidth = ImGui::CalcTextSize(l_Summary.c_str()).x;
            float l_CountersWidth = ImGui::CalcTextSize(ICON_TR_WARNING " 000  " ICON_TR_ERROR " 000").x;
            float l_Total = l_SummaryWidth + l_CountersWidth + l_IconWidth * 2.0f + 40.0f;

            ImGui::SameLine(ImGui::GetWindowWidth() - l_Total);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%s", l_Summary.c_str());

            ImGui::SameLine(0.0f, 16.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::ToVec4(l_Warnings > 0 ? EditorColors::WarningText : EditorColors::DisabledText));
            ImGui::TextUnformatted((std::string(ICON_TR_WARNING " ") + std::to_string(l_Warnings)).c_str());
            ImGui::PopStyleColor();

            ImGui::SameLine(0.0f, 10.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::ToVec4(l_Errors > 0 ? EditorColors::ErrorText : EditorColors::DisabledText));
            ImGui::TextUnformatted((std::string(ICON_TR_ERROR " ") + std::to_string(l_Errors)).c_str());
            ImGui::PopStyleColor();

            ImGui::SameLine(0.0f, 12.0f);
            if (EditorTheme::ToolBarButton(ICON_TR_MUTE, m_Context.MuteAudio, "Mute Audio"))
            {
                m_Context.MuteAudio = !m_Context.MuteAudio;
            }

            ImGui::SameLine();
            if (EditorTheme::ToolBarButton(ICON_TR_GIZMOS, m_Context.ShowPhysicsColliders, "Gizmos"))
            {
                m_Context.ShowPhysicsColliders = !m_Context.ShowPhysicsColliders;
            }
        }

        EditorTheme::EndChromeBar();

        m_Context.ChromeBottom += l_Height;
    }
}