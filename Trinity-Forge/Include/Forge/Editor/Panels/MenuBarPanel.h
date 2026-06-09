#pragma once

#include <cstdint>

#include <Forge/Editor/EditorPanel.h>

namespace Trinity
{
    class Window;

    class MenuBarPanel : public EditorPanel
    {
    public:
        MenuBarPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;

    private:
        void HandleShortcuts();
        void RenderMenus();
        void RenderMenuBar();
        void RenderTitleBar(Window& window);
        void EnsureLogo();

        uint64_t m_LogoTexture = 0;
        bool m_LogoTried = false;
    };
}