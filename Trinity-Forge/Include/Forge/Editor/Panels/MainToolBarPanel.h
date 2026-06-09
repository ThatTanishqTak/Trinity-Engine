#pragma once

#include <Forge/Editor/EditorPanel.h>

namespace Trinity
{
    class MainToolBarPanel : public EditorPanel
    {
    public:
        MainToolBarPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;
    };
}