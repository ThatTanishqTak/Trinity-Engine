#pragma once

#include "Forge/Panels/Panel.h"
#include "Forge/SelectionContext.h"

namespace Forge
{
    class SceneHierarchyPanel : public Panel
    {
    public:
        SceneHierarchyPanel(std::string name, SelectionContext* context);

        void OnRender() override;

    private:
        void RenderEntityNode(entt::entity entity);

        SelectionContext* m_Context = nullptr;
        entt::entity m_RenameTarget = entt::null;
        bool  m_RenameRequested = false;
    };
}