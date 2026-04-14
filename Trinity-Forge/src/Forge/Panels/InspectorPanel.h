#pragma once

#include "Forge/Panels/Panel.h"

struct SelectionContext;

namespace Forge
{
    class InspectorPanel : public Panel
    {
    public:
        InspectorPanel(std::string name, SelectionContext* context);

        void OnRender() override;

    private:
        SelectionContext* m_Context = nullptr;
    };
}
