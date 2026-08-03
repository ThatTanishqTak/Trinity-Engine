#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

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
        void DragTransformField(Scene& scene, uint64_t uuid, const char* label, TransformComponent& transform, glm::vec3& value, float resetValue, float speed);
        void DragRotationField(Scene& scene, uint64_t uuid, TransformComponent& transform);
        void DrawMultiSelectionInspector(Scene& scene);

    private:
        std::string m_RenameOldName;
        TransformComponent m_TransformEditOld;
        std::vector<std::pair<uint64_t, TransformComponent>> m_MultiEditOld;
        glm::vec3 m_RotationEditCache{ 0.0f };
        uint64_t m_RotationEditTarget = 0;
        bool m_RotationEditActive = false;
        char m_DetailsSearch[128] = "";
    };
}