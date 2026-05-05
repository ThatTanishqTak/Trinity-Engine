#include "Trinity/Renderer/RenderGraph/RenderGraph.h"

#include "Trinity/Renderer/CommandList.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Utilities/Log.h"

#include <algorithm>
#include <limits>

namespace Trinity
{
    std::shared_ptr<Texture> RenderGraphContext::GetTexture(RenderGraphResourceHandle handle) const
    {
        return Graph ? Graph->GetTexture(handle) : nullptr;
    }

    CommandList& RenderGraphContext::GetCommandList() const
    {
        return *Cmd;
    }

    RenderGraphResourceHandle RenderGraph::DeclareTexture(const RenderGraphTextureDescription& description)
    {
        RenderGraphResourceHandle l_Handle{};
        l_Handle.Index = static_cast<uint32_t>(m_ResourceDescription.size());

        m_ResourceDescription.push_back(description);
        m_Compiled = false;

        return l_Handle;
    }

    RenderGraphPass& RenderGraph::AddPass(const std::string& name)
    {
        m_Passes.emplace_back(name);
        m_Compiled = false;

        TR_CORE_TRACE("RenderGraphPass Added: {}", name);

        return m_Passes.back();
    }

    void RenderGraph::Reset()
    {
        TR_CORE_TRACE("Resetting Render Graph");

        OnReset();

        m_ResourceDescription.clear();
        m_Resources.clear();
        m_Passes.clear();
        m_ExecutionOrder.clear();
        m_Compiled = false;

        TR_CORE_TRACE("Render Graph Reset Complete");
    }

    void RenderGraph::Compile()
    {
        OnReset();

        m_ExecutionOrder.clear();
        m_Resources.clear();
        m_Compiled = false;

        BuildExecutionOrder();
        AllocateResources();

        OnCompile();

        m_Compiled = true;
    }

    void RenderGraph::Execute()
    {
        if (!m_Compiled)
        {
            Compile();
        }

        RenderGraphContext l_Context{};
        l_Context.Graph = this;
        l_Context.Cmd = &Renderer::GetCommandList();
        l_Context.Width = m_SwapchainWidth;
        l_Context.Height = m_SwapchainHeight;

        for (uint32_t it_Index : m_ExecutionOrder)
        {
            OnExecutePassBegin(it_Index, l_Context);

            m_Passes[it_Index].Execute(l_Context);

            OnExecutePassEnd(it_Index, l_Context);
        }
    }

    void RenderGraph::OnResize(uint32_t width, uint32_t height)
    {
        if (m_SwapchainWidth == width && m_SwapchainHeight == height)
        {
            return;
        }

        m_SwapchainWidth = width;
        m_SwapchainHeight = height;

        m_Compiled = false;
    }

    std::shared_ptr<Texture> RenderGraph::GetTexture(RenderGraphResourceHandle handle) const
    {
        if (!handle.IsValid())
        {
            return nullptr;
        }

        if (handle.Index >= m_Resources.size())
        {
            return nullptr;
        }

        return m_Resources[handle.Index];
    }

    void RenderGraph::BuildExecutionOrder()
    {
        const uint32_t l_PassCount = static_cast<uint32_t>(m_Passes.size());
        if (l_PassCount == 0)
        {
            return;
        }

        const uint32_t l_ResourceCount = static_cast<uint32_t>(m_ResourceDescription.size());
        constexpr uint32_t l_InvalidPass = std::numeric_limits<uint32_t>::max();

        std::vector<std::vector<uint32_t>> l_Dependencies(l_PassCount);
        std::vector<uint32_t> l_InDegree(l_PassCount, 0);
        std::vector<uint32_t> l_LastWriter(l_ResourceCount, l_InvalidPass);
        std::vector<std::vector<uint32_t>> l_ActiveReaders(l_ResourceCount);

        auto a_AddDependency = [&](uint32_t from, uint32_t to)
        {
            if (from == to)
            {
                return;
            }

            auto& a_Dependency = l_Dependencies[to];
            if (std::find(a_Dependency.begin(), a_Dependency.end(), from) == a_Dependency.end())
            {
                a_Dependency.push_back(from);
            }
        };

        for (uint32_t i = 0; i < l_PassCount; i++)
        {
            const auto& a_Pass = m_Passes[i];

            for (const auto& it_Handle : a_Pass.GetReads())
            {
                if (!it_Handle.IsValid() || it_Handle.Index >= l_ResourceCount)
                {
                    continue;
                }

                if (l_LastWriter[it_Handle.Index] != l_InvalidPass)
                {
                    a_AddDependency(l_LastWriter[it_Handle.Index], i);
                }
            }

            for (const auto& it_Handle : a_Pass.GetWrites())
            {
                if (!it_Handle.IsValid() || it_Handle.Index >= l_ResourceCount)
                {
                    continue;
                }

                if (l_LastWriter[it_Handle.Index] != l_InvalidPass)
                {
                    a_AddDependency(l_LastWriter[it_Handle.Index], i);
                }

                for (uint32_t l_Reader : l_ActiveReaders[it_Handle.Index])
                {
                    a_AddDependency(l_Reader, i);
                }
            }

            for (const auto& it_Handle : a_Pass.GetReads())
            {
                if (!it_Handle.IsValid() || it_Handle.Index >= l_ResourceCount)
                {
                    continue;
                }

                l_ActiveReaders[it_Handle.Index].push_back(i);
            }

            for (const auto& it_Handle : a_Pass.GetWrites())
            {
                if (!it_Handle.IsValid() || it_Handle.Index >= l_ResourceCount)
                {
                    continue;
                }

                l_LastWriter[it_Handle.Index] = i;
                l_ActiveReaders[it_Handle.Index].clear();
            }
        }

        for (uint32_t i = 0; i < l_PassCount; i++)
        {
            l_InDegree[i] = static_cast<uint32_t>(l_Dependencies[i].size());
        }

        std::vector<bool> l_Visited(l_PassCount, false);
        m_ExecutionOrder.reserve(l_PassCount);

        for (uint32_t l_Step = 0; l_Step < l_PassCount; l_Step++)
        {
            uint32_t l_Next = l_InvalidPass;
            for (uint32_t i = 0; i < l_PassCount; i++)
            {
                if (!l_Visited[i] && l_InDegree[i] == 0)
                {
                    l_Next = i;
                    break;
                }
            }

            if (l_Next == l_InvalidPass)
            {
                m_ExecutionOrder.clear();

                return;
            }

            l_Visited[l_Next] = true;
            m_ExecutionOrder.push_back(l_Next);

            for (uint32_t i = 0; i < l_PassCount; i++)
            {
                if (l_Visited[i])
                {
                    continue;
                }

                for (uint32_t it_Dependency : l_Dependencies[i])
                {
                    if (it_Dependency == l_Next)
                    {
                        l_InDegree[i]--;
                        break;
                    }
                }
            }
        }
    }

    void RenderGraph::AllocateResources()
    {
        m_Resources.assign(m_ResourceDescription.size(), nullptr);

        for (size_t i = 0; i < m_ResourceDescription.size(); i++)
        {
            RenderGraphTextureDescription l_ResolveDescription = m_ResourceDescription[i];
            if (l_ResolveDescription.Width == 0)
            {
                l_ResolveDescription.Width = m_SwapchainWidth;
            }

            if (l_ResolveDescription.Height == 0)
            {
                l_ResolveDescription.Height = m_SwapchainHeight;
            }

            if (l_ResolveDescription.Width == 0 || l_ResolveDescription.Height == 0)
            {
                continue;
            }

            m_Resources[i] = CreateResource(l_ResolveDescription);
        }
    }
}