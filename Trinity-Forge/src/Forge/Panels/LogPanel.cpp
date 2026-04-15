#include "Forge/Panels/LogPanel.h"

#include <imgui.h>

namespace Forge
{
    LogPanel::LogPanel(std::string name) : Panel(std::move(name))
    {

    }

    void LogPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        ImGui::End();
    }
}
