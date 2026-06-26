#include <Forge/Editor/Panels/RenderGraphPanel.h>

#include <string>
#include <vector>

#include <imgui.h>

#include <Forge/Editor/EditorContext.h>

#include <Trinity/Core/Engine.h>
#include <Trinity/Renderer/Frontend/Renderer.h>
#include <Trinity/Renderer/Graph/RenderGraph.h>
#include <Trinity/Renderer/RHI/GraphicsDevice.h>
#include <Trinity/ImGui/IImGuiRenderBackend.h>

namespace Trinity
{
    void RenderGraphPanel::OnImGuiRender()
    {
        // Drive the renderer's depth pass only while the panel is open.
        if (m_Engine.HasRenderer())
        {
            m_Engine.GetRenderer().SetDepthVisualizationEnabled(m_Context.ShowRenderGraph);
        }

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

        DrawPasses();

        ImGui::Spacing();
        ImGui::SeparatorText("Render Targets");
        DrawRenderTargets();

        ImGui::End();
    }

    void RenderGraphPanel::DrawPasses()
    {
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
    }

    void RenderGraphPanel::DrawRenderTargets()
    {
        if (!m_Engine.HasDevice())
        {
            return;
        }

        IImGuiRenderBackend& l_Backend = m_Engine.GetDevice().GetImGuiBackend();
        std::vector<DebugRenderTarget> l_Targets = m_Engine.GetRenderer().GetDebugRenderTargets();

        for (const DebugRenderTarget& it_Target : l_Targets)
        {
            PreviewEntry* l_Entry = nullptr;
            for (PreviewEntry& it_Existing : m_Previews)
            {
                if (it_Existing.Name == it_Target.Name)
                {
                    l_Entry = &it_Existing;

                    break;
                }
            }

            if (l_Entry == nullptr)
            {
                m_Previews.push_back({ it_Target.Name, TextureHandle{}, 0 });
                l_Entry = &m_Previews.back();
            }

            // The handle changes when the target is recreated and re-register so the ImGui descriptor points at the live image view
            if (l_Entry->Handle != it_Target.Texture)
            {
                if (l_Entry->TextureID != 0)
                {
                    l_Backend.UnregisterTexture(l_Entry->TextureID);
                }

                l_Entry->TextureID = l_Backend.RegisterTexture(it_Target.Texture);
                l_Entry->Handle = it_Target.Texture;
            }

            if (l_Entry->TextureID == 0)
            {
                continue;
            }

            if (ImGui::CollapsingHeader(it_Target.Name, ImGuiTreeNodeFlags_DefaultOpen))
            {
                float l_Available = ImGui::GetContentRegionAvail().x;
                float l_Aspect = it_Target.Height > 0 ? static_cast<float>(it_Target.Width) / static_cast<float>(it_Target.Height) : (16.0f / 9.0f);
                if (l_Aspect <= 0.0f)
                {
                    l_Aspect = 16.0f / 9.0f;
                }

                ImVec2 l_Size(l_Available, l_Available / l_Aspect);
                ImGui::Image((ImTextureID)l_Entry->TextureID, l_Size);
                ImGui::TextDisabled("%ux%u", it_Target.Width, it_Target.Height);
            }
        }
    }
}