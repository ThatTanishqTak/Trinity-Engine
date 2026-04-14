#include "Forge/Panels/SceneHierarchyPanel.h"

#include <imgui.h>

namespace Forge
{
    SceneHierarchyPanel::SceneHierarchyPanel(std::string name, SelectionContext* context)
        : Panel(std::move(name)), m_Context(context)
    {
    }

    void SceneHierarchyPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        ImGui::End();
    }
}
