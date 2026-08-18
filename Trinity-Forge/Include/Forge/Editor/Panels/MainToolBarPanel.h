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

    private:
        void ProcessRequests();
        void RenderLeftCluster();
        void RenderPlayControls(float barWidth);
        void RenderRightCluster(float barWidth);
        void TogglePlay();
        void TogglePause();

        char m_SearchBuffer[128] = "";
    };
}