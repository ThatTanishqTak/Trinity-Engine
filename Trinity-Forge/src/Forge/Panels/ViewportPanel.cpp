#include "Forge/Panels/ViewportPanel.h"

#include <imgui.h>

namespace Forge
{
    ViewportPanel::ViewportPanel(std::string name)
        : Panel(std::move(name))
    {
    }

    void ViewportPanel::OnRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(m_Name.c_str(), &m_Open);
        ImGui::PopStyleVar();

        ImGui::End();
    }
}
