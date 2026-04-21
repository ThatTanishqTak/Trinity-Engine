#include "Forge/Panels/RendererStatsPanel.h"

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Utilities/Time.h"

#include <imgui.h>

namespace Forge
{
    RendererStatsPanel::RendererStatsPanel(std::string name) : Panel(std::move(name))
    {

    }

    void RendererStatsPanel::OnRender()
    {
        ImGui::Begin(m_Name.c_str(), &m_Open);

        const float l_DeltaTime = Trinity::CoreUtilities::Time::DeltaTime();
        const float l_FPS = l_DeltaTime > 0.0f ? 1.0f / l_DeltaTime : 0.0f;

        ImGui::Text("Backend:    Vulkan");
        ImGui::Text("Frame:      %u / %u", Trinity::Renderer::GetCurrentFrameIndex(), Trinity::Renderer::GetMaxFramesInFlight());
        ImGui::Text("Frame Time: %.3f ms", l_DeltaTime * 1000.0f);
        ImGui::Text("FPS:        %.1f", l_FPS);

        ImGui::Separator();

        // These require VkQueryPool timestamp queries, gap until SceneRenderer exposes them
        ImGui::TextDisabled("Draw Calls:     --");
        ImGui::TextDisabled("Vertices:       --");
        ImGui::TextDisabled("Geometry Pass:  -- ms");
        ImGui::TextDisabled("Shadow Pass:    -- ms");
        ImGui::TextDisabled("GPU Total:      -- ms");

        ImGui::End();
    }
}