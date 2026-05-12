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
        m_Lifetimes.clear();
        m_Compiled = false;

        BuildExecutionOrder();
        ComputeLifetimes();
        AllocateResources();

        OnCompile();

        m_Compiled = true;
    }

    void RenderGraph::Execute()
    {
        if (!m_Compiled)
        {
            Compile();

            if (m_ExecutionOrder.empty() && !m_Passes.empty())
            {
                TR_CORE_ERROR("RenderGraph execution skipped because no valid execution order exists");
                return;
            }
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
                TR_CORE_ERROR("RenderGraph dependency cycle detected Execution order could not be built");
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

    void RenderGraph::ComputeLifetimes()
    {
        const uint32_t l_ResourceCount = static_cast<uint32_t>(m_ResourceDescription.size());
        m_Lifetimes.assign(l_ResourceCount, ResourceLifetime{});

        if (l_ResourceCount == 0 || m_ExecutionOrder.empty())
        {
            return;
        }

        for (uint32_t l_OrderIdx = 0; l_OrderIdx < m_ExecutionOrder.size(); ++l_OrderIdx)
        {
            const uint32_t l_PassIdx = m_ExecutionOrder[l_OrderIdx];
            if (l_PassIdx >= m_Passes.size())
            {
                continue;
            }

            const auto& a_Pass = m_Passes[l_PassIdx];

            auto a_Touch = [&](uint32_t resourceIdx)
            {
                if (resourceIdx >= l_ResourceCount)
                {
                    return;
                }

                ResourceLifetime& a_Lifetime = m_Lifetimes[resourceIdx];
                if (l_OrderIdx < a_Lifetime.FirstUse)
                {
                    a_Lifetime.FirstUse = l_OrderIdx;
                }
                if (l_OrderIdx > a_Lifetime.LastUse)
                {
                    a_Lifetime.LastUse = l_OrderIdx;
                }
            };

            for (const auto& it_Handle : a_Pass.GetReads())
            {
                if (it_Handle.IsValid())
                {
                    a_Touch(it_Handle.Index);
                }
            }
            for (const auto& it_Handle : a_Pass.GetWrites())
            {
                if (it_Handle.IsValid())
                {
                    a_Touch(it_Handle.Index);
                }
            }
        }

        for (uint32_t i = 0; i < l_ResourceCount; ++i)
        {
            if (!m_Lifetimes[i].IsValid())
            {
                TR_CORE_WARN("Resource '{}' (index {}) declared but never used by any pass", m_ResourceDescription[i].DebugName, i);
            }
        }
    }

    void RenderGraph::AllocateResources()
    {
        m_Resources = AllocateResourceBatch(m_ResourceDescription, m_Lifetimes);
    }
}