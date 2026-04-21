#pragma once

#include "Forge/Panels/Panel.h"
#include "Forge/SelectionContext.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <imgui.h>
#include <string>

namespace Forge
{
    class InspectorPanel : public Panel
    {
    public:
        InspectorPanel(std::string name, SelectionContext* context);

        void OnRender() override;

    private:
        template<typename T, typename DrawFunction>
        static void DrawComponent(const char* label, entt::registry& registry, entt::entity entity, DrawFunction drawFunction)
        {
            if (!registry.all_of<T>(entity))
            {
                return;
            }

            const ImGuiTreeNodeFlags l_Flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

            auto& a_Component = registry.get<T>(entity);

            ImGui::PushID(reinterpret_cast<const void*>(&typeid(T)));
            const bool l_Open = ImGui::TreeNodeEx(label, l_Flags);

            bool l_Remove = false;
            if (ImGui::BeginPopupContextItem("##ComponentContext"))
            {
                if (ImGui::MenuItem("Remove Component"))
                {
                    l_Remove = true;
                }

                ImGui::EndPopup();
            }

            if (l_Open)
            {
                drawFunction(a_Component);
                ImGui::TreePop();
            }

            ImGui::PopID();

            if (l_Remove)
            {
                registry.remove<T>(entity);
            }
        }

        static void DrawVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f);

        void DrawAddComponentPopup(entt::registry& registry, entt::entity entity);

        SelectionContext* m_Context = nullptr;
        char m_ComponentSearchBuffer[128] = {};
    };
}
