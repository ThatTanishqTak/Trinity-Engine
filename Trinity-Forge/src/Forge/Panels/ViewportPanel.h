#pragma once

#include "Forge/Panels/Panel.h"

namespace Forge
{
    class ViewportPanel : public Panel
    {
    public:
        explicit ViewportPanel(std::string name);

        void OnRender() override;
    };
}
