#pragma once

#include "Forge/Panels/Panel.h"

struct SelectionContext;

namespace Forge
{
    class SceneHierarchyPanel : public Panel
    {
    public:
        SceneHierarchyPanel(std::string name, SelectionContext* context);

        void OnRender() override;

    private:
        SelectionContext* m_Context = nullptr;
    };
}
