#pragma once

#include "Forge/Panels/Panel.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Forge
{
    class PanelManager
    {
    public:
        void Initialize();
        void Shutdown();

        template<typename T, typename... Args>
        T* RegisterPanel(Args&&... args)
        {
            auto l_Panel = std::make_unique<T>(std::forward<Args>(args)...);
            T* l_Ptr = l_Panel.get();
            l_Panel->OnInitialize();
            m_Panels.push_back(std::move(l_Panel));

            return l_Ptr;
        }

        void UpdatePanels(float deltaTime);
        void PreRenderPanels();
        void RenderPanels();
        void RenderViewMenu();

        Panel* FindPanel(const std::string& name) const;

    private:
        std::vector<std::unique_ptr<Panel>> m_Panels;
    };
}
