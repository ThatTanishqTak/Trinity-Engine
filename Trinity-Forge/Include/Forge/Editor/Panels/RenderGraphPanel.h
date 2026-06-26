#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Trinity/Renderer/RHI/Handle.h>

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

    private:
        void DrawPasses();
        void DrawRenderTargets();

        struct PreviewEntry
        {
            std::string Name;
            TextureHandle Handle;
            uint64_t TextureID = 0;
        };

        std::vector<PreviewEntry> m_Previews;
    };
}