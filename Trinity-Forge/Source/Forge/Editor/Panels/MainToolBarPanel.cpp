#include <Forge/Editor/Panels/MainToolBarPanel.h>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Audio/Frontend/AudioEngine.h>
#include <Trinity/Scene/Scene.h>

namespace Trinity
{
    void MainToolBarPanel::OnImGuiRender()
    {
        ImGuiStyle& l_Style = ImGui::GetStyle();

        float l_ButtonSize = ImGui::GetFrameHeight() * 1.3f;
        float l_PadY = 6.0f;
        float l_Height = l_ButtonSize + l_PadY * 2.0f;

        ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(l_Viewport->Pos.x, l_Viewport->Pos.y + m_Context.ChromeTop));
        ImGui::SetNextWindowSize(ImVec2(l_Viewport->Size.x, l_Height));
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        ImGuiWindowFlags l_Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, l_PadY));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 0.0f));
        ImGui::Begin("##ForgeMainToolbar", nullptr, l_Flags);
        ImGui::PopStyleVar(4);

        ImGui::SetWindowFontScale(1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        if (ImGui::Button(ICON_FA_FLOPPY_DISK, ImVec2(l_ButtonSize, l_ButtonSize)))
        {
            m_Context.FileOp = PendingFileOp::Save;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Save  (Ctrl+S)");
        }

        ImGui::SameLine();
        ImVec2 l_SepPos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(l_SepPos.x, l_SepPos.y + 3.0f), ImVec2(l_SepPos.x, l_SepPos.y + l_ButtonSize - 3.0f), ImGui::GetColorU32(ImGuiCol_Separator));
        ImGui::SameLine(0.0f, l_Style.ItemSpacing.x + 8.0f);

        bool l_Playing = m_Context.PlayMode;

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.60f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.72f, 0.34f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.52f, 0.24f, 1.0f));
        ImGui::BeginDisabled(l_Playing);
        if (ImGui::Button(ICON_FA_PLAY, ImVec2(l_ButtonSize, l_ButtonSize)))
        {
            m_Context.PlayMode = true;
            if (m_Engine.HasScene() && m_Engine.HasAssetDatabase() && m_Engine.HasAudioEngine())
            {
                m_Engine.GetAudioEngine().StartScene(m_Engine.GetScene(), m_Engine.GetAssetDatabase());
            }
        }
        ImGui::EndDisabled();
        if (!l_Playing && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Play");
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.24f, 0.24f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.82f, 0.30f, 0.30f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.20f, 0.20f, 1.0f));
        ImGui::BeginDisabled(!l_Playing);
        if (ImGui::Button(ICON_FA_STOP, ImVec2(l_ButtonSize, l_ButtonSize)))
        {
            m_Context.PlayMode = false;
            if (m_Engine.HasScene() && m_Engine.HasAudioEngine())
            {
                m_Engine.GetAudioEngine().StopScene(m_Engine.GetScene());
            }
        }
        ImGui::EndDisabled();
        if (l_Playing && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Stop");
        }
        ImGui::PopStyleColor(3);

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::SetWindowFontScale(1.0f);

        ImGui::End();

        m_Context.ChromeTop += l_Height;
    }
}