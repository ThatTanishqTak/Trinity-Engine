#pragma once

#include "Forge/Panels/Panel.h"

namespace Forge
{
    class RendererStatsPanel : public Panel
    {
    public:
        explicit RendererStatsPanel(std::string name);

        void OnRender() override;
    };
}