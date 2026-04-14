#pragma once

#include "Forge/Panels/Panel.h"

namespace Forge
{
    class ContentBrowserPanel : public Panel
    {
    public:
        explicit ContentBrowserPanel(std::string name);

        void OnRender() override;
    };
}
