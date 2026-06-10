#include <Forge/Editor/Panels/ConsolePanel.h>

#include <imgui.h>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    void ConsolePanel::OnImGuiRender()
    {
        if (ImGui::Begin("Console"))
        {
            RenderContents();
        }

        ImGui::End();
    }

    void ConsolePanel::RenderContents()
    {
        if (ImGui::Button("Clear"))
        {
            Log::ClearMessages();
        }

        ImGui::SameLine();
        ImGui::Checkbox("Autoscroll", &m_Autoscroll);
        ImGui::Separator();

        ImGui::BeginChild("ConsoleScroll", ImVec2(0.0f, 0.0f), 0, ImGuiWindowFlags_HorizontalScrollbar);

        Log::VisitMessages([](const LogMessage& message)
        {
            ImVec4 l_Color(0.85f, 0.85f, 0.85f, 1.0f);
            switch (message.Level)
            {
                case LogLevel::Trace:
                    l_Color = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
                    break;
                case LogLevel::Info:
                    l_Color = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
                    break;
                case LogLevel::Warn:
                    l_Color = ImVec4(1.0f, 0.78f, 0.30f, 1.0f);
                    break;
                case LogLevel::Error:
                    l_Color = ImVec4(1.0f, 0.40f, 0.40f, 1.0f);
                    break;
                case LogLevel::Critical:
                    l_Color = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
                    break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, l_Color);
            ImGui::TextUnformatted(message.Text.c_str());
            ImGui::PopStyleColor();
        });

        if (m_Autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
    }
}