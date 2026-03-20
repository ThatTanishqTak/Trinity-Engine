#include "Trinity/Renderer/Vulkan/VulkanRenderGraph.h"

namespace Trinity
{
    void VulkanRenderGraph::Execute(const VulkanFrameContext& frameContext)
    {
        for (auto& it_Pass : m_Passes)
        {
            it_Pass->Execute(frameContext);
        }
    }

    void VulkanRenderGraph::Recreate(uint32_t width, uint32_t height)
    {
        for (auto& it_Pass : m_Passes)
        {
            it_Pass->Recreate(width, height);
        }
    }

    void VulkanRenderGraph::Shutdown()
    {
        for (auto it = m_Passes.rbegin(); it != m_Passes.rend(); ++it)
        {
            (*it)->Shutdown();
        }

        m_Passes.clear();
        m_PassMap.clear();
    }
}