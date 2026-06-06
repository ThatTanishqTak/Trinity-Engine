#pragma once

#include <entt/entt.hpp>

#include <Forge/Editor/EditorPanel.h>

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
    };
}