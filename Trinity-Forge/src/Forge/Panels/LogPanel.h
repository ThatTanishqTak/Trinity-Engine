#pragma once

#include "Forge/Panels/Panel.h"

namespace Forge
{
    class LogPanel : public Panel
    {
    public:
        explicit LogPanel(std::string name);

        void OnRender() override;
    };
}
