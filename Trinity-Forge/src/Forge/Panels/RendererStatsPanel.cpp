#include "Forge/Panels/RendererStatsPanel.h"

#include "Trinity/Renderer/Renderer.h"

#include <imgui.h>

namespace Forge
{
    RendererStatsPanel::RendererStatsPanel(std::string name) : Panel(std::move(name))
    {

    }

    void RendererStatsPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        ImGui::Text("Backend: Vulkan");
        ImGui::Text("Frame Index: %u / %u", Trinity::Renderer::GetCurrentFrameIndex(), Trinity::Renderer::GetMaxFramesInFlight());

        ImGui::End();
    }
}
