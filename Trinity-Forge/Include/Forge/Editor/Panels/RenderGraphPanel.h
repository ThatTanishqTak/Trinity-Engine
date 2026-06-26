#pragma once

#include <Forge/Editor/EditorPanel.h>

namespace Trinity
{
    class RenderGraphPanel : public EditorPanel
    {
    public:
        RenderGraphPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;
    };
}