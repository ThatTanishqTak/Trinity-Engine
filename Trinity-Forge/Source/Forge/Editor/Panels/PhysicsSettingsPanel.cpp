#include <Forge/Editor/Panels/PhysicsSettingsPanel.h>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>
#include <Forge/Editor/EditorIcons.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Core/SimulationClock.h>
#include <Trinity/Physics/Frontend/PhysicsSystem.h>

namespace Trinity
{
    void PhysicsSettingsPanel::OnImGuiRender()
    {
        if (!m_Context.ShowPhysicsSettings)
        {
            return;
        }

        if (!m_Engine.HasPhysicsSystem())
        {
            m_Context.ShowPhysicsSettings = false;

            return;
        }

        ImGui::SetNextWindowSize(ImVec2(420.0f, 480.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(ICON_TR_SETTINGS "  Physics", &m_Context.ShowPhysicsSettings))
        {
            ImGui::End();

            return;
        }

        PhysicsSettings& l_Settings = m_Engine.GetPhysicsSystem().GetSettings();

        ImGui::TextDisabled("Changes apply when the next play session starts.");
        ImGui::Spacing();

        ImGui::DragFloat2("Gravity 2D", &l_Settings.Gravity2D.x, 0.1f);
        ImGui::DragFloat3("Gravity 3D", &l_Settings.Gravity3D.x, 0.1f);

        int l_Hertz = static_cast<int>(1.0f / m_Engine.GetSimulationClock().GetFixedDelta() + 0.5f);
        if (ImGui::SliderInt("Fixed Step (Hz)", &l_Hertz, 30, 240))
        {
            float l_Delta = 1.0f / static_cast<float>(l_Hertz);
            m_Engine.GetSimulationClock().SetFixedDelta(l_Delta);
            l_Settings.FixedDelta = l_Delta;
        }

        int l_SubSteps = static_cast<int>(l_Settings.SolverSubSteps);
        if (ImGui::SliderInt("Solver Sub-Steps", &l_SubSteps, 1, 8))
        {
            l_Settings.SolverSubSteps = static_cast<uint32_t>(l_SubSteps);
        }

        ImGui::DragFloat("Sleep Velocity", &l_Settings.SleepLinearVelocity, 0.01f, 0.0f, 1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted("Layer Collision Matrix");

        constexpr uint32_t k_VisibleLayers = 8;
        ImGui::TextDisabled("Layers 0-%u shown; every layer collides by default.", k_VisibleLayers - 1);

        if (ImGui::BeginTable("##LayerMatrix", static_cast<int>(k_VisibleLayers) + 1, ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            for (uint32_t it_Column = 0; it_Column < k_VisibleLayers; ++it_Column)
            {
                ImGui::TableNextColumn();
                ImGui::Text("%u", it_Column);
            }

            // Upper triangle only; the matrix is kept symmetric on edit.
            for (uint32_t it_Row = 0; it_Row < k_VisibleLayers; ++it_Row)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%u", it_Row);

                for (uint32_t it_Column = 0; it_Column < k_VisibleLayers; ++it_Column)
                {
                    ImGui::TableNextColumn();
                    if (it_Column < it_Row)
                    {
                        continue;
                    }

                    bool l_Collides = (l_Settings.LayerCollisionMatrix[it_Row] & (1u << it_Column)) != 0;
                    ImGui::PushID(static_cast<int>(it_Row * 32 + it_Column));
                    if (ImGui::Checkbox("##Collide", &l_Collides))
                    {
                        if (l_Collides)
                        {
                            l_Settings.LayerCollisionMatrix[it_Row] |= (1u << it_Column);
                            l_Settings.LayerCollisionMatrix[it_Column] |= (1u << it_Row);
                        }
                        else
                        {
                            l_Settings.LayerCollisionMatrix[it_Row] &= ~(1u << it_Column);
                            l_Settings.LayerCollisionMatrix[it_Column] &= ~(1u << it_Row);
                        }
                    }
                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }
}