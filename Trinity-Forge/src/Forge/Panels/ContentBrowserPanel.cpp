#include "Forge/Panels/ContentBrowserPanel.h"

#include <imgui.h>

namespace Forge
{
    ContentBrowserPanel::ContentBrowserPanel(std::string name) : Panel(std::move(name))
    {

    }

    void ContentBrowserPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        ImGui::End();
    }
}
