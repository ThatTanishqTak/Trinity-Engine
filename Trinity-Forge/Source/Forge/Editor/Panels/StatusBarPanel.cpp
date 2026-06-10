#include <Forge/Editor/Panels/StatusBarPanel.h>

#include <string>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/IDComponent.h>

namespace Trinity
{
    void StatusBarPanel::OnImGuiRender()
    {
        float l_Height = ImGui::GetFrameHeight() + 6.0f;

        ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(l_Viewport->Pos.x, l_Viewport->Pos.y + l_Viewport->Size.y - l_Height));
        ImGui::SetNextWindowSize(ImVec2(l_Viewport->Size.x, l_Height));
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        ImGuiWindowFlags l_Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 3.0f));
        ImGui::Begin("##ForgeStatusBar", nullptr, l_Flags);
        ImGui::PopStyleVar(3);

        if (ImGui::SmallButton(ICON_FA_FOLDER_OPEN " Content Drawer"))
        {
            bool l_Show = !m_Context.ShowContentDrawer;
            m_Context.ShowContentDrawer = l_Show;
            if (l_Show)
            {
                m_Context.ShowConsoleDrawer = false;
            }
            m_Context.DrawerToggled = true;
        }

        ImGui::SameLine();

        if (ImGui::SmallButton(ICON_FA_TERMINAL " Console"))
        {
            bool l_Show = !m_Context.ShowConsoleDrawer;
            m_Context.ShowConsoleDrawer = l_Show;
            if (l_Show)
            {
                m_Context.ShowContentDrawer = false;
            }
            m_Context.DrawerToggled = true;
        }

        int l_EntityCount = 0;
        if (m_Engine.HasScene())
        {
            l_EntityCount = static_cast<int>(m_Engine.GetScene().GetRegistry().view<IDComponent>().size());
        }

        std::string l_State = m_Context.History.IsDirty() ? "Unsaved *" : "All Saved";
        std::string l_Right = std::to_string(l_EntityCount) + " entities    |    " + l_State;

        float l_RightWidth = ImGui::CalcTextSize(l_Right.c_str()).x;
        ImGui::SameLine(ImGui::GetWindowWidth() - l_RightWidth - 12.0f);
        ImGui::TextUnformatted(l_Right.c_str());

        ImGui::End();

        m_Context.ChromeBottom += l_Height;
    }
}