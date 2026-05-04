#pragma once

#include "Forge/Panels/Panel.h"

#include "Trinity/Utilities/Log.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Forge
{
    class PanelManager
    {
    public:
        template<typename T, typename... Args>
        T* RegisterPanel(Args&&... args)
        {
            auto l_Panel = std::make_unique<T>(std::forward<Args>(args)...);
            T* l_Pointer = l_Panel.get();
            l_Panel->OnInitialize();

            TR_TRACE("Registered Panel: {}", l_Panel->GetName());
            m_Panels.push_back(std::move(l_Panel));

            return l_Pointer;
        }
        void DeregisterPanel();

        void UpdatePanels(float deltaTime);
        void PreRenderPanels();
        void RenderPanels();
        void RenderViewMenu();

        Panel* FindPanel(const std::string& name) const;

    private:
        std::vector<std::unique_ptr<Panel>> m_Panels;
    };
}