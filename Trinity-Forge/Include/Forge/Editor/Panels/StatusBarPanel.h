#pragma once

#include <Forge/Editor/EditorPanel.h>

namespace Trinity
{
    class StatusBarPanel : public EditorPanel
    {
    public:
        StatusBarPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;
    };
}