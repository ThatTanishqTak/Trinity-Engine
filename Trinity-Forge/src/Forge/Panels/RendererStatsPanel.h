#pragma once

#include "Forge/Panels/Panel.h"
#include "Trinity/Renderer/SceneRenderer.h"

#include <functional>

namespace Forge
{
    class RendererStatsPanel : public Panel
    {
    public:
        explicit RendererStatsPanel(std::string name);

        void OnRender() override;

        void SetStatsProvider(std::function<const Trinity::SceneRendererStats* ()> provider);

    private:
        std::function<const Trinity::SceneRendererStats* ()> m_StatsProvider;
    };
}