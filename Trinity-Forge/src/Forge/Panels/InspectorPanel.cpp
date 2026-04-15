#include "Forge/Panels/InspectorPanel.h"

#include <imgui.h>

namespace Forge
{
    InspectorPanel::InspectorPanel(std::string name, SelectionContext* context) : Panel(std::move(name)), m_Context(context)
    {

    }

    void InspectorPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        ImGui::End();
    }
}
