#pragma once

#include <vector>

#include <entt/entt.hpp>

#include <Forge/Editor/EditorPanel.h>

#include <Forge/Editor/EditorTheme.h>
#include <Forge/Editor/EditorIcons.h>

namespace Trinity
{
    class Scene;

    class HierarchyPanel : public EditorPanel
    {
    public:
        HierarchyPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;

    private:
        void RenderEntityNode(Scene& scene, entt::entity entity);
        void RenderFlatRow(Scene& scene, entt::entity entity);
        void HandleRowClick(entt::entity entity);
        void DrawRowStripe();

        int m_RowIndex = 0;
        char m_SearchBuffer[128] = "";
        entt::entity m_SelectionAnchor = entt::null;
        // Rows in draw order; shift-range-select uses last frame's complete list.
        std::vector<entt::entity> m_RowsThisFrame;
        std::vector<entt::entity> m_RowsLastFrame;
    };
}