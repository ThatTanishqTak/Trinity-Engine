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
        bool m_ShowInfo = true;
        bool m_ShowWarnings = true;
        bool m_ShowErrors = true;
        int m_SelectedMessage = -1;
    };
}