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
        float l_Height = ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.y * 2.0f + 4.0f;

        ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(l_Viewport->Pos.x, l_Viewport->Pos.y + m_Context.ChromeTop));
        ImGui::SetNextWindowSize(ImVec2(l_Viewport->Size.x, l_Height));
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        ImGuiWindowFlags l_Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("##ForgeMainToolbar", nullptr, l_Flags);
        ImGui::PopStyleVar(2);

        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save"))
        {
            m_Context.FileOp = PendingFileOp::Save;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        bool l_Playing = m_Context.PlayMode;

        ImGui::BeginDisabled(l_Playing);
        if (ImGui::Button(ICON_FA_PLAY " Play"))
        {
            m_Context.PlayMode = true;
            if (m_Engine.HasScene() && m_Engine.HasAssetDatabase() && m_Engine.HasAudioEngine())
            {
                m_Engine.GetAudioEngine().StartScene(m_Engine.GetScene(), m_Engine.GetAssetDatabase());
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!l_Playing);
        if (ImGui::Button(ICON_FA_STOP " Stop"))
        {
            m_Context.PlayMode = false;
            if (m_Engine.HasScene() && m_Engine.HasAudioEngine())
            {
                m_Engine.GetAudioEngine().StopScene(m_Engine.GetScene());
            }
        }
        ImGui::EndDisabled();

        ImGui::End();

        m_Context.ChromeTop += l_Height;
    }
}