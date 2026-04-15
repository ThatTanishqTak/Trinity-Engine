#include "Forge/Panels/PanelManager.h"

#include <imgui.h>

namespace Forge
{
    void PanelManager::Initialize()
    {

    }

    void PanelManager::Shutdown()
    {
        for (auto& it_Panel : m_Panels)
        {
            it_Panel->OnShutdown();
        }

        m_Panels.clear();
    }

    void PanelManager::UpdatePanels(float deltaTime)
    {
        for (auto& it_Panel : m_Panels)
        {
            if (it_Panel->IsOpen())
            {
                it_Panel->OnUpdate(deltaTime);
            }
        }
    }

    void PanelManager::RenderPanels()
    {
        for (auto& it_Panel : m_Panels)
        {
            if (it_Panel->IsOpen())
            {
                it_Panel->OnRender();
            }
        }
    }

    void PanelManager::RenderViewMenu()
    {
        for (auto& it_Panel : m_Panels)
        {
            ImGui::MenuItem(it_Panel->GetName().c_str(), nullptr, &it_Panel->GetOpenState());
        }
    }

    Panel* PanelManager::FindPanel(const std::string& name) const
    {
        for (const auto& it_Panel : m_Panels)
        {
            if (it_Panel->GetName() == name)
            {
                return it_Panel.get();
            }
        }

        return nullptr;
    }
}
