#pragma once

#include <Forge/Editor/EditorPanel.h>

namespace Trinity
{
    class ContentBrowserPanel : public EditorPanel
    {
    public:
        ContentBrowserPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;

    private:
        void ReResolveModifiedMeshes();
    };
}