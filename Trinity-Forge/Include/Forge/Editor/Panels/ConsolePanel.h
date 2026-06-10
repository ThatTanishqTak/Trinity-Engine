#pragma once

#include <Forge/Editor/EditorPanel.h>

namespace Trinity
{
    class ConsolePanel : public EditorPanel
    {
    public:
        ConsolePanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;
        void RenderContents();

    private:
        bool m_Autoscroll = true;
    };
}