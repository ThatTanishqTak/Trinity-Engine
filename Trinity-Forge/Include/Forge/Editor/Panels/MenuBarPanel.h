#pragma once

#include <Forge/Editor/EditorPanel.h>

namespace Trinity
{
    class MenuBarPanel : public EditorPanel
    {
    public:
        MenuBarPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;

    private:
        void HandleShortcuts();
        void RenderMenuBar();
    };
}