#pragma once

#include <Forge/Editor/EditorPanel.h>

namespace Trinity
{
    class PhysicsSettingsPanel : public EditorPanel
    {
    public:
        PhysicsSettingsPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;
    };
}