#include <Forge/Editor/Panels/RenderGraphPanel.h>

#include <string>
#include <vector>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Renderer/Frontend/Renderer.h>
#include <Trinity/Renderer/Graph/RenderGraph.h>

namespace Trinity
{
    void RenderGraphPanel::OnImGuiRender()
    {
        if (!m_Context.ShowRenderGraph)
        {
            return;
        }

        if (!ImGui::Begin("Render Graph", &m_Context.ShowRenderGraph))
        {
            ImGui::End();

            return;
        }

        if (!m_Engine.HasRenderer())
        {
            ImGui::TextUnformatted("Renderer not initialized.");
            ImGui::End();

            return;
        }

        const std::vector<RenderGraph::PassInfo>& l_Passes = m_Engine.GetRenderer().GetRenderGraph().GetPasses();

        ImGui::Text("Passes: %d", static_cast<int>(l_Passes.size()));
        ImGui::TextDisabled("Resource transitions are derived automatically from declared reads/writes.");
        ImGui::Separator();

        ImGuiTableFlags l_Flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
        if (ImGui::BeginTable("##RenderGraphPasses", 4, l_Flags))
        {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 28.0f);
            ImGui::TableSetupColumn("Pass");
            ImGui::TableSetupColumn("Reads");
            ImGui::TableSetupColumn("Writes");
            ImGui::TableHeadersRow();

            for (size_t l_Index = 0; l_Index < l_Passes.size(); ++l_Index)
            {
                const RenderGraph::PassInfo& l_Pass = l_Passes[l_Index];

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", static_cast<int>(l_Index));

                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(l_Pass.Name.c_str());
                if (!l_Pass.Managed)
                {
                    ImGui::SameLine();
                    ImGui::TextDisabled("(raw)");
                }

                ImGui::TableSetColumnIndex(2);
                if (l_Pass.Reads.empty())
                {
                    ImGui::TextDisabled("-");
                }
                else
                {
                    for (const std::string& it_Read : l_Pass.Reads)
                    {
                        ImGui::TextUnformatted(it_Read.c_str());
                    }
                }

                ImGui::TableSetColumnIndex(3);
                if (l_Pass.Writes.empty())
                {
                    ImGui::TextDisabled("-");
                }
                else
                {
                    for (const std::string& it_Write : l_Pass.Writes)
                    {
                        ImGui::TextUnformatted(it_Write.c_str());
                    }
                }
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }
}