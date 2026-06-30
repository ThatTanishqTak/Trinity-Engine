#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

#include <Forge/Editor/EditorPanel.h>

#include <Trinity/Scene/Components/TransformComponent.h>

namespace Trinity
{
    class Scene;

    class InspectorPanel : public EditorPanel
    {
    public:
        InspectorPanel(EditorContext& context, Engine& engine) : EditorPanel(context, engine)
        {

        }

        void OnImGuiRender() override;

    private:
        void DragTransformField(Scene& scene, uint64_t uuid, const char* label, TransformComponent& transform, glm::vec3& value, float speed);

        std::string m_RenameOldName;
        TransformComponent m_TransformEditOld;
        char m_DetailsSearch[128] = "";
    };
}