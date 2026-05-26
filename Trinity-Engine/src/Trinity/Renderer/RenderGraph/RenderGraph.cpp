#include "Trinity/Renderer/RenderGraph/RenderGraph.h"

#include "Trinity/Renderer/CommandList.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Utilities/Log.h"

#include <algorithm>
#include <limits>
#include <queue>
#include <sstream>
#include <unordered_set>

namespace Trinity
{
    std::shared_ptr<Texture> RenderGraphContext::GetTexture(RenderGraphResourceHandle handle) const
    {
        return Graph ? Graph->GetTexture(handle) : nullptr;
    }

    std::shared_ptr<Buffer> RenderGraphContext::GetBuffer(RenderGraphResourceHandle handle) const
    {
        return Graph ? Graph->GetBuffer(handle) : nullptr;
    }

    CommandList& RenderGraphContext::GetCommandList() const
    {
        return *Command;
    }

    RenderGraphResourceHandle RenderGraph::DeclareTexture(const RenderGraphTextureDescription& description)
    {
        RenderGraphResourceHandle l_Handle = RenderGraphResourceHandle::Texture(static_cast<uint32_t>(m_TextureDescriptions.size()));

        RenderGraphTextureDescription l_Description = description;
        if (l_Description.DebugName.empty())
        {
            l_Description.DebugName = "Texture_" + std::to_string(l_Handle.Index);
        }

        m_TextureDescriptions.push_back(l_Description);
        m_Textures.push_back(nullptr);
        m_TextureInitialAccesses.push_back(RenderGraphAccess::None);

        m_NamedResources[l_Description.DebugName] = l_Handle;
        m_Compiled = false;

        return l_Handle;
    }

    RenderGraphResourceHandle RenderGraph::DeclareBuffer(const RenderGraphBufferDescription& description)
    {
        RenderGraphResourceHandle l_Handle = RenderGraphResourceHandle::Buffer(static_cast<uint32_t>(m_BufferDescriptions.size()));

        RenderGraphBufferDescription l_Description = description;
        if (l_Description.DebugName.empty())
        {
            l_Description.DebugName = "Buffer_" + std::to_string(l_Handle.Index);
        }

        m_BufferDescriptions.push_back(l_Description);
        m_Buffers.push_back(nullptr);
        m_BufferInitialAccesses.push_back(RenderGraphAccess::None);

        m_NamedResources[l_Description.DebugName] = l_Handle;
        m_Compiled = false;

        return l_Handle;
    }

    RenderGraphResourceHandle RenderGraph::ImportTexture(const std::string& name, const std::shared_ptr<Texture>& texture, RenderGraphAccess initialAccess)
    {
        RenderGraphTextureDescription l_Description{};

        if (texture)
        {
            const TextureSpecification& l_Specification = texture->GetSpecification();

            l_Description.Width = l_Specification.Width;
            l_Description.Height = l_Specification.Height;
            l_Description.MipLevels = l_Specification.MipLevels;
            l_Description.ArrayLayers = l_Specification.ArrayLayers;
            l_Description.SampleCount = l_Specification.Samples;
            l_Description.Format = l_Specification.Format;
            l_Description.Usage = l_Specification.Usage;
            l_Description.MatchSwapchainSize = false;
            l_Description.DebugName = !name.empty() ? name : l_Specification.DebugName;
        }
        else
        {
            l_Description.DebugName = !name.empty() ? name : "ImportedTexture";
        }

        l_Description.Persistent = true;
        l_Description.Imported = true;

        RenderGraphResourceHandle l_Handle = RenderGraphResourceHandle::Texture(static_cast<uint32_t>(m_TextureDescriptions.size()));

        m_TextureDescriptions.push_back(l_Description);
        m_Textures.push_back(texture);
        m_TextureInitialAccesses.push_back(initialAccess);

        m_NamedResources[l_Description.DebugName] = l_Handle;
        m_Compiled = false;

        return l_Handle;
    }

    RenderGraphResourceHandle RenderGraph::ImportBuffer(const std::string& name, const std::shared_ptr<Buffer>& buffer, RenderGraphAccess initialAccess)
    {
        RenderGraphBufferDescription l_Description{};

        if (buffer)
        {
            const BufferSpecification& l_Specification = buffer->GetSpecification();

            l_Description.Size = l_Specification.Size;
            l_Description.Usage = l_Specification.Usage;
            l_Description.MemoryType = l_Specification.MemoryType;
            l_Description.DebugName = !name.empty() ? name : l_Specification.DebugName;
        }
        else
        {
            l_Description.DebugName = !name.empty() ? name : "ImportedBuffer";
        }

        l_Description.Persistent = true;
        l_Description.Imported = true;

        RenderGraphResourceHandle l_Handle = RenderGraphResourceHandle::Buffer(static_cast<uint32_t>(m_BufferDescriptions.size()));

        m_BufferDescriptions.push_back(l_Description);
        m_Buffers.push_back(buffer);
        m_BufferInitialAccesses.push_back(initialAccess);

        m_NamedResources[l_Description.DebugName] = l_Handle;
        m_Compiled = false;

        return l_Handle;
    }

    void RenderGraph::SetImportedTexture(RenderGraphResourceHandle handle, const std::shared_ptr<Texture>& texture)
    {
        if (!handle.IsTexture())
        {
            TR_CORE_WARN("SetImportedTexture: handle is not a texture");
            return;
        }

        if (handle.Index >= m_TextureDescriptions.size())
        {
            TR_CORE_WARN("SetImportedTexture: handle index {} out of range", handle.Index);
            return;
        }

        if (!m_TextureDescriptions[handle.Index].Imported)
        {
            TR_CORE_WARN("SetImportedTexture: texture '{}' is not flagged as imported", m_TextureDescriptions[handle.Index].DebugName);
            return;
        }

        m_Textures[handle.Index] = texture;

        if (texture)
        {
            const TextureSpecification& l_Specification = texture->GetSpecification();
            RenderGraphTextureDescription& l_Description = m_TextureDescriptions[handle.Index];

            l_Description.Width = l_Specification.Width;
            l_Description.Height = l_Specification.Height;
            l_Description.Format = l_Specification.Format;
            l_Description.Usage = l_Specification.Usage;
            l_Description.MipLevels = l_Specification.MipLevels;
            l_Description.ArrayLayers = l_Specification.ArrayLayers;
            l_Description.SampleCount = l_Specification.Samples;
        }
    }

    void RenderGraph::MarkOutput(RenderGraphResourceHandle handle)
    {
        if (!IsValidHandle(handle))
        {
            TR_CORE_ERROR("RenderGraph::MarkOutput called with invalid handle");
            return;
        }

        if (handle.IsTexture())
        {
            m_TextureDescriptions[handle.Index].ExternalOutput = true;
        }
        else if (handle.IsBuffer())
        {
            m_BufferDescriptions[handle.Index].ExternalOutput = true;
        }

        m_Compiled = false;
    }

    RenderGraphPass& RenderGraph::AddPass(const std::string& name)
    {
        auto it_Existing = m_NamedPasses.find(name);
        if (it_Existing != m_NamedPasses.end())
        {
            TR_CORE_WARN("RenderGraph::AddPass duplicate pass name '{}'", name);
        }

        const uint32_t l_Index = static_cast<uint32_t>(m_Passes.size());

        m_Passes.emplace_back(name);
        m_NamedPasses[name] = l_Index;

        m_Compiled = false;

        TR_CORE_TRACE("RenderGraphPass Added: {}", name);

        return m_Passes.back();
    }

    void RenderGraph::Reset()
    {
        TR_CORE_TRACE("Resetting Render Graph");

        OnReset();

        m_TextureDescriptions.clear();
        m_BufferDescriptions.clear();

        m_Textures.clear();
        m_Buffers.clear();

        m_TextureInitialAccesses.clear();
        m_BufferInitialAccesses.clear();

        m_Passes.clear();
        m_ExecutionOrder.clear();

        m_TextureLifetimes.clear();
        m_BufferLifetimes.clear();

        m_ValidationErrors.clear();

        m_NamedResources.clear();
        m_NamedPasses.clear();

        m_Compiled = false;

        TR_CORE_TRACE("Render Graph Reset Complete");
    }

    bool RenderGraph::Validate()
    {
        m_ValidationErrors.clear();

        auto AddError = [this](const std::string& message)
        {
            m_ValidationErrors.push_back(message);
        };

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_TextureDescriptions.size()); ++i)
        {
            const auto& l_Description = m_TextureDescriptions[i];

            if (l_Description.Format == TextureFormat::None)
            {
                AddError("Texture '" + l_Description.DebugName + "' has TextureFormat::None");
            }

            if (l_Description.Usage == TextureUsage::None)
            {
                AddError("Texture '" + l_Description.DebugName + "' has TextureUsage::None");
            }

            if (!l_Description.MatchSwapchainSize && (l_Description.Width == 0 || l_Description.Height == 0))
            {
                AddError("Texture '" + l_Description.DebugName + "' has zero size and MatchSwapchainSize=false");
            }

            if (l_Description.Imported && !m_Textures[i])
            {
                AddError("Imported texture '" + l_Description.DebugName + "' has null texture pointer");
            }
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_BufferDescriptions.size()); ++i)
        {
            const auto& l_Description = m_BufferDescriptions[i];

            if (l_Description.Size == 0)
            {
                AddError("Buffer '" + l_Description.DebugName + "' has size 0");
            }

            if (l_Description.Usage == BufferUsage::None)
            {
                AddError("Buffer '" + l_Description.DebugName + "' has BufferUsage::None");
            }

            if (l_Description.Imported && !m_Buffers[i])
            {
                AddError("Imported buffer '" + l_Description.DebugName + "' has null buffer pointer");
            }
        }

        std::unordered_set<std::string> l_PassNames;
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_Passes.size()); ++i)
        {
            const RenderGraphPass& l_Pass = m_Passes[i];

            if (l_Pass.GetName().empty())
            {
                AddError("Pass at index " + std::to_string(i) + " has empty name");
            }

            if (!l_PassNames.insert(l_Pass.GetName()).second)
            {
                AddError("Duplicate pass name '" + l_Pass.GetName() + "'");
            }

            const bool l_HasResources = !l_Pass.GetReads().empty() || !l_Pass.GetWrites().empty() || !l_Pass.GetReadWrites().empty();

            if (l_Pass.IsEnabled() && !l_Pass.HasExecuteCallback() && l_HasResources)
            {
                AddError("Pass '" + l_Pass.GetName() + "' uses resources but has no execute callback");
            }

            auto ValidateUsageList = [&](const std::vector<RenderGraphResourceUsage>& usages, const char* usageName)
            {
                for (const RenderGraphResourceUsage& it_Usage : usages)
                {
                    if (!it_Usage.Resource.IsValid())
                    {
                        AddError("Pass '" + l_Pass.GetName() + "' has invalid " + usageName + " resource");
                        continue;
                    }

                    if (!IsValidHandle(it_Usage.Resource))
                    {
                        AddError("Pass '" + l_Pass.GetName() + "' references out-of-range " + usageName + " resource");
                    }

                    if (it_Usage.Access == RenderGraphAccess::None)
                    {
                        AddError("Pass '" + l_Pass.GetName() + "' uses resource '" + GetResourceName(it_Usage.Resource) + "' with RenderGraphAccess::None");
                    }
                }
            };

            ValidateUsageList(l_Pass.GetReads(), "read");
            ValidateUsageList(l_Pass.GetWrites(), "write");
            ValidateUsageList(l_Pass.GetReadWrites(), "read-write");
        }

        if (!m_ValidationErrors.empty())
        {
            TR_CORE_ERROR("RenderGraph validation failed with {} error(s)", m_ValidationErrors.size());
            for (const std::string& it_Error : m_ValidationErrors)
            {
                TR_CORE_ERROR("  {}", it_Error);
            }

            return false;
        }

        return true;
    }

    void RenderGraph::Compile()
    {
        OnReset();

        m_ExecutionOrder.clear();
        m_TextureLifetimes.clear();
        m_BufferLifetimes.clear();

        m_Compiled = false;

        if (!Validate())
        {
            return;
        }

        BuildExecutionOrder();

        if (m_ExecutionOrder.empty() && !m_Passes.empty())
        {
            TR_CORE_ERROR("RenderGraph compile failed: no valid execution order was produced");
            return;
        }

        CullPasses();
        ComputeLifetimes();
        AllocateResources();

        OnCompile();

        m_Compiled = true;

        TR_CORE_TRACE("RenderGraph compiled with {} pass(es), {} texture(s), {} buffer(s)", m_ExecutionOrder.size(), m_TextureDescriptions.size(), m_BufferDescriptions.size());
    }

    void RenderGraph::Execute()
    {
        if (!m_Compiled)
        {
            Compile();

            if (!m_Compiled)
            {
                TR_CORE_ERROR("RenderGraph execution skipped because compile failed");
                return;
            }
        }

        RenderGraphContext l_Context{};
        l_Context.Graph = this;
        l_Context.Command = &Renderer::GetCommandList();
        l_Context.Width = m_SwapchainWidth;
        l_Context.Height = m_SwapchainHeight;

        for (uint32_t it_Index : m_ExecutionOrder)
        {
            if (it_Index >= m_Passes.size())
            {
                continue;
            }

            RenderGraphPass& l_Pass = m_Passes[it_Index];
            if (!l_Pass.IsEnabled())
            {
                continue;
            }

            l_Context.PassIndex = it_Index;
            l_Context.PassName = l_Pass.GetName().c_str();

            OnExecutePassBegin(it_Index, l_Context);

            l_Pass.Execute(l_Context);

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
        if (!handle.IsTexture())
        {
            return nullptr;
        }

        if (handle.Index >= m_Textures.size())
        {
            return nullptr;
        }

        return m_Textures[handle.Index];
    }

    std::shared_ptr<Buffer> RenderGraph::GetBuffer(RenderGraphResourceHandle handle) const
    {
        if (!handle.IsBuffer())
        {
            return nullptr;
        }

        if (handle.Index >= m_Buffers.size())
        {
            return nullptr;
        }

        return m_Buffers[handle.Index];
    }

    RenderGraphResourceHandle RenderGraph::GetResourceHandle(const std::string& name) const
    {
        auto it_Resource = m_NamedResources.find(name);
        if (it_Resource == m_NamedResources.end())
        {
            return RenderGraphResourceHandle::Invalid();
        }

        return it_Resource->second;
    }

    RenderGraphPass* RenderGraph::GetPass(const std::string& name)
    {
        auto it_Pass = m_NamedPasses.find(name);
        if (it_Pass == m_NamedPasses.end() || it_Pass->second >= m_Passes.size())
        {
            return nullptr;
        }

        return &m_Passes[it_Pass->second];
    }

    const RenderGraphPass* RenderGraph::GetPass(const std::string& name) const
    {
        auto it_Pass = m_NamedPasses.find(name);
        if (it_Pass == m_NamedPasses.end() || it_Pass->second >= m_Passes.size())
        {
            return nullptr;
        }

        return &m_Passes[it_Pass->second];
    }

    std::string RenderGraph::GetResourceName(RenderGraphResourceHandle handle) const
    {
        if (!handle.IsValid())
        {
            return "<invalid>";
        }

        if (handle.IsTexture())
        {
            if (handle.Index < m_TextureDescriptions.size())
            {
                return m_TextureDescriptions[handle.Index].DebugName;
            }

            return "<invalid texture>";
        }

        if (handle.IsBuffer())
        {
            if (handle.Index < m_BufferDescriptions.size())
            {
                return m_BufferDescriptions[handle.Index].DebugName;
            }

            return "<invalid buffer>";
        }

        return "<unknown>";
    }

    std::string RenderGraph::DumpText() const
    {
        std::ostringstream l_Output;

        l_Output << "RenderGraph\n";
        l_Output << "Textures: " << m_TextureDescriptions.size() << "\n";
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_TextureDescriptions.size()); ++i)
        {
            const auto& l_Description = m_TextureDescriptions[i];
            l_Output << "  [" << i << "] " << l_Description.DebugName << " imported=" << (l_Description.Imported ? "true" : "false") << " persistent=" << (l_Description.Persistent ? "true" : "false") << " output=" << (l_Description.ExternalOutput ? "true" : "false") << "\n";
        }

        l_Output << "Buffers: " << m_BufferDescriptions.size() << "\n";
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_BufferDescriptions.size()); ++i)
        {
            const auto& l_Description = m_BufferDescriptions[i];
            l_Output << "  [" << i << "] " << l_Description.DebugName << " size=" << l_Description.Size << " imported=" << (l_Description.Imported ? "true" : "false") << " persistent=" << (l_Description.Persistent ? "true" : "false") << " output=" << (l_Description.ExternalOutput ? "true" : "false") << "\n";
        }

        l_Output << "Passes: " << m_Passes.size() << "\n";
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_Passes.size()); ++i)
        {
            const RenderGraphPass& l_Pass = m_Passes[i];

            l_Output << "  [" << i << "] " << l_Pass.GetName() << " type=" << PassTypeToString(l_Pass.GetType()) << " enabled=" << (l_Pass.IsEnabled() ? "true" : "false") << " cullable=" << (l_Pass.IsCullable() ? "true" : "false") << "\n";

            for (const auto& it_Read : l_Pass.GetReads())
            {
                l_Output << "    Read: " << GetResourceName(it_Read.Resource) << " as " << AccessToString(it_Read.Access) << "\n";
            }

            for (const auto& it_Write : l_Pass.GetWrites())
            {
                l_Output << "    Write: " << GetResourceName(it_Write.Resource) << " as " << AccessToString(it_Write.Access) << "\n";
            }

            for (const auto& it_ReadWrite : l_Pass.GetReadWrites())
            {
                l_Output << "    ReadWrite: " << GetResourceName(it_ReadWrite.Resource) << " as " << AccessToString(it_ReadWrite.Access) << "\n";
            }
        }

        l_Output << "ExecutionOrder:";
        for (uint32_t it_Index : m_ExecutionOrder)
        {
            if (it_Index < m_Passes.size())
            {
                l_Output << " " << m_Passes[it_Index].GetName();
            }
        }
        l_Output << "\n";

        return l_Output.str();
    }

    std::string RenderGraph::DumpDot() const
    {
        std::ostringstream l_Output;

        l_Output << "digraph RenderGraph {\n";
        l_Output << "  rankdir=LR;\n";

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_Passes.size()); ++i)
        {
            l_Output << "  pass_" << i << " [shape=box,label=\"" << m_Passes[i].GetName() << "\"];\n";
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_TextureDescriptions.size()); ++i)
        {
            l_Output << "  tex_" << i << " [shape=ellipse,label=\"" << m_TextureDescriptions[i].DebugName << "\"];\n";
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_BufferDescriptions.size()); ++i)
        {
            l_Output << "  buf_" << i << " [shape=ellipse,label=\"" << m_BufferDescriptions[i].DebugName << "\"];\n";
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_Passes.size()); ++i)
        {
            const RenderGraphPass& l_Pass = m_Passes[i];

            auto EmitResourceNode = [&l_Output](RenderGraphResourceHandle handle) -> std::string
            {
                std::ostringstream l_Node;
                if (handle.IsTexture())
                {
                    l_Node << "tex_" << handle.Index;
                }
                else if (handle.IsBuffer())
                {
                    l_Node << "buf_" << handle.Index;
                }
                else
                {
                    l_Node << "invalid";
                }

                return l_Node.str();
            };

            for (const auto& it_Read : l_Pass.GetReads())
            {
                l_Output << "  " << EmitResourceNode(it_Read.Resource) << " -> pass_" << i << ";\n";
            }

            for (const auto& it_Write : l_Pass.GetWrites())
            {
                l_Output << "  pass_" << i << " -> " << EmitResourceNode(it_Write.Resource) << ";\n";
            }

            for (const auto& it_ReadWrite : l_Pass.GetReadWrites())
            {
                l_Output << "  " << EmitResourceNode(it_ReadWrite.Resource) << " -> pass_" << i << ";\n";
                l_Output << "  pass_" << i << " -> " << EmitResourceNode(it_ReadWrite.Resource) << ";\n";
            }
        }

        l_Output << "}\n";

        return l_Output.str();
    }

    bool RenderGraph::IsValidHandle(RenderGraphResourceHandle handle) const
    {
        if (!handle.IsValid())
        {
            return false;
        }

        if (handle.IsTexture())
        {
            return handle.Index < m_TextureDescriptions.size();
        }

        if (handle.IsBuffer())
        {
            return handle.Index < m_BufferDescriptions.size();
        }

        return false;
    }

    void RenderGraph::BuildExecutionOrder()
    {
        const uint32_t l_PassCount = static_cast<uint32_t>(m_Passes.size());
        if (l_PassCount == 0)
        {
            return;
        }

        constexpr uint32_t l_InvalidPass = std::numeric_limits<uint32_t>::max();

        std::vector<std::vector<uint32_t>> l_Dependencies(l_PassCount);
        std::vector<std::vector<uint32_t>> l_Dependents(l_PassCount);
        std::vector<uint32_t> l_InDegree(l_PassCount, 0);

        std::vector<uint32_t> l_LastTextureWriter(m_TextureDescriptions.size(), l_InvalidPass);
        std::vector<uint32_t> l_LastBufferWriter(m_BufferDescriptions.size(), l_InvalidPass);

        auto AddDependency = [&](uint32_t from, uint32_t to)
        {
            if (from == to || from == l_InvalidPass || to == l_InvalidPass)
            {
                return;
            }

            auto& l_List = l_Dependencies[to];
            if (std::find(l_List.begin(), l_List.end(), from) == l_List.end())
            {
                l_List.push_back(from);
                l_Dependents[from].push_back(to);
            }
        };

        auto ProcessRead = [&](uint32_t passIndex, RenderGraphResourceHandle handle)
        {
            if (!IsValidHandle(handle))
            {
                return;
            }

            if (handle.IsTexture())
            {
                AddDependency(l_LastTextureWriter[handle.Index], passIndex);
            }
            else if (handle.IsBuffer())
            {
                AddDependency(l_LastBufferWriter[handle.Index], passIndex);
            }
        };

        auto ProcessWrite = [&](uint32_t passIndex, RenderGraphResourceHandle handle)
        {
            if (!IsValidHandle(handle))
            {
                return;
            }

            if (handle.IsTexture())
            {
                AddDependency(l_LastTextureWriter[handle.Index], passIndex);
                l_LastTextureWriter[handle.Index] = passIndex;
            }
            else if (handle.IsBuffer())
            {
                AddDependency(l_LastBufferWriter[handle.Index], passIndex);
                l_LastBufferWriter[handle.Index] = passIndex;
            }
        };

        for (uint32_t i = 0; i < l_PassCount; ++i)
        {
            const RenderGraphPass& l_Pass = m_Passes[i];

            if (!l_Pass.IsEnabled())
            {
                continue;
            }

            for (const auto& it_Read : l_Pass.GetReads())
            {
                ProcessRead(i, it_Read.Resource);
            }

            for (const auto& it_ReadWrite : l_Pass.GetReadWrites())
            {
                ProcessRead(i, it_ReadWrite.Resource);
            }

            for (const auto& it_Write : l_Pass.GetWrites())
            {
                ProcessWrite(i, it_Write.Resource);
            }

            for (const auto& it_ReadWrite : l_Pass.GetReadWrites())
            {
                ProcessWrite(i, it_ReadWrite.Resource);
            }
        }

        for (uint32_t i = 0; i < l_PassCount; ++i)
        {
            l_InDegree[i] = static_cast<uint32_t>(l_Dependencies[i].size());
        }

        std::queue<uint32_t> l_ReadyQueue;
        for (uint32_t i = 0; i < l_PassCount; ++i)
        {
            if (m_Passes[i].IsEnabled() && l_InDegree[i] == 0)
            {
                l_ReadyQueue.push(i);
            }
        }

        while (!l_ReadyQueue.empty())
        {
            uint32_t l_Current = l_ReadyQueue.front();
            l_ReadyQueue.pop();

            m_ExecutionOrder.push_back(l_Current);

            for (uint32_t it_Dependent : l_Dependents[l_Current])
            {
                if (l_InDegree[it_Dependent] > 0)
                {
                    --l_InDegree[it_Dependent];
                }

                if (l_InDegree[it_Dependent] == 0 && m_Passes[it_Dependent].IsEnabled())
                {
                    l_ReadyQueue.push(it_Dependent);
                }
            }
        }

        uint32_t l_EnabledPassCount = 0;
        for (const RenderGraphPass& it_Pass : m_Passes)
        {
            if (it_Pass.IsEnabled())
            {
                ++l_EnabledPassCount;
            }
        }

        if (m_ExecutionOrder.size() != l_EnabledPassCount)
        {
            TR_CORE_ERROR("RenderGraph dependency cycle detected. Execution order could not be built.");
            m_ExecutionOrder.clear();
        }
    }

    void RenderGraph::CullPasses()
    {
        if (m_ExecutionOrder.empty())
        {
            return;
        }

        std::vector<bool> l_Needed(m_Passes.size(), false);

        auto IsOutputUsage = [this](const RenderGraphResourceUsage& usage)
        {
            return IsResourceMarkedOutput(usage.Resource);
        };

        bool l_AnyOutputMarked = false;
        for (const auto& it_Description : m_TextureDescriptions)
        {
            l_AnyOutputMarked |= it_Description.ExternalOutput;
        }

        for (const auto& it_Description : m_BufferDescriptions)
        {
            l_AnyOutputMarked |= it_Description.ExternalOutput;
        }

        if (!l_AnyOutputMarked)
        {
            return;
        }

        std::vector<RenderGraphResourceHandle> l_NeededResources;

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_TextureDescriptions.size()); ++i)
        {
            if (m_TextureDescriptions[i].ExternalOutput)
            {
                l_NeededResources.push_back(RenderGraphResourceHandle::Texture(i));
            }
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_BufferDescriptions.size()); ++i)
        {
            if (m_BufferDescriptions[i].ExternalOutput)
            {
                l_NeededResources.push_back(RenderGraphResourceHandle::Buffer(i));
            }
        }

        for (auto it_Order = m_ExecutionOrder.rbegin(); it_Order != m_ExecutionOrder.rend(); ++it_Order)
        {
            const uint32_t l_PassIndex = *it_Order;
            RenderGraphPass& l_Pass = m_Passes[l_PassIndex];

            bool l_PassNeeded = !l_Pass.IsCullable();

            for (const auto& it_Write : l_Pass.GetWrites())
            {
                if (std::find(l_NeededResources.begin(), l_NeededResources.end(), it_Write.Resource) != l_NeededResources.end())
                {
                    l_PassNeeded = true;
                    break;
                }
            }

            if (!l_PassNeeded)
            {
                for (const auto& it_ReadWrite : l_Pass.GetReadWrites())
                {
                    if (std::find(l_NeededResources.begin(), l_NeededResources.end(), it_ReadWrite.Resource) != l_NeededResources.end())
                    {
                        l_PassNeeded = true;
                        break;
                    }
                }
            }

            if (!l_PassNeeded)
            {
                continue;
            }

            l_Needed[l_PassIndex] = true;

            for (const auto& it_Read : l_Pass.GetReads())
            {
                l_NeededResources.push_back(it_Read.Resource);
            }

            for (const auto& it_ReadWrite : l_Pass.GetReadWrites())
            {
                l_NeededResources.push_back(it_ReadWrite.Resource);
            }
        }

        std::vector<uint32_t> l_CulledOrder;
        l_CulledOrder.reserve(m_ExecutionOrder.size());

        for (uint32_t it_PassIndex : m_ExecutionOrder)
        {
            if (it_PassIndex < l_Needed.size() && l_Needed[it_PassIndex])
            {
                l_CulledOrder.push_back(it_PassIndex);
            }
            else
            {
                TR_CORE_TRACE("RenderGraph culled pass '{}'", m_Passes[it_PassIndex].GetName());
            }
        }

        m_ExecutionOrder = std::move(l_CulledOrder);
    }

    void RenderGraph::ComputeLifetimes()
    {
        m_TextureLifetimes.assign(m_TextureDescriptions.size(), {});
        m_BufferLifetimes.assign(m_BufferDescriptions.size(), {});

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_TextureDescriptions.size()); ++i)
        {
            m_TextureLifetimes[i].Imported = m_TextureDescriptions[i].Imported;
            m_TextureLifetimes[i].Persistent = m_TextureDescriptions[i].Persistent;
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_BufferDescriptions.size()); ++i)
        {
            m_BufferLifetimes[i].Imported = m_BufferDescriptions[i].Imported;
            m_BufferLifetimes[i].Persistent = m_BufferDescriptions[i].Persistent;
        }

        for (uint32_t l_OrderIndex = 0; l_OrderIndex < static_cast<uint32_t>(m_ExecutionOrder.size()); ++l_OrderIndex)
        {
            const uint32_t l_PassIndex = m_ExecutionOrder[l_OrderIndex];
            if (l_PassIndex >= m_Passes.size())
            {
                continue;
            }

            const RenderGraphPass& l_Pass = m_Passes[l_PassIndex];

            for (const auto& it_Read : l_Pass.GetReads())
            {
                TouchResourceLifetime(it_Read.Resource, l_OrderIndex, false);
            }

            for (const auto& it_Write : l_Pass.GetWrites())
            {
                TouchResourceLifetime(it_Write.Resource, l_OrderIndex, true);
            }

            for (const auto& it_ReadWrite : l_Pass.GetReadWrites())
            {
                TouchResourceLifetime(it_ReadWrite.Resource, l_OrderIndex, true);
            }
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_TextureDescriptions.size()); ++i)
        {
            if (!m_TextureDescriptions[i].Imported && !m_TextureLifetimes[i].IsValid())
            {
                TR_CORE_WARN("RenderGraph texture '{}' declared but never used", m_TextureDescriptions[i].DebugName);
            }
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_BufferDescriptions.size()); ++i)
        {
            if (!m_BufferDescriptions[i].Imported && !m_BufferLifetimes[i].IsValid())
            {
                TR_CORE_WARN("RenderGraph buffer '{}' declared but never used", m_BufferDescriptions[i].DebugName);
            }
        }
    }

    void RenderGraph::AllocateResources()
    {
        std::vector<std::shared_ptr<Texture>> l_AllocatedTextures = AllocateResourceBatch(m_TextureDescriptions, m_TextureLifetimes);

        if (l_AllocatedTextures.size() == m_TextureDescriptions.size())
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(m_TextureDescriptions.size()); ++i)
            {
                if (m_TextureDescriptions[i].Imported)
                {
                    continue;
                }

                m_Textures[i] = l_AllocatedTextures[i];
            }
        }

        if (m_Buffers.size() != m_BufferDescriptions.size())
        {
            m_Buffers.resize(m_BufferDescriptions.size());
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_BufferDescriptions.size()); ++i)
        {
            if (m_BufferDescriptions[i].Imported)
            {
                continue;
            }

            m_Buffers[i] = AllocateBuffer(m_BufferDescriptions[i]);
        }
    }

    void RenderGraph::TouchResourceLifetime(RenderGraphResourceHandle handle, uint32_t orderIndex, bool writer)
    {
        if (!IsValidHandle(handle))
        {
            return;
        }

        RenderGraphResourceLifetime* l_Lifetime = nullptr;

        if (handle.IsTexture())
        {
            l_Lifetime = &m_TextureLifetimes[handle.Index];
        }
        else if (handle.IsBuffer())
        {
            l_Lifetime = &m_BufferLifetimes[handle.Index];
        }

        if (!l_Lifetime)
        {
            return;
        }

        if (orderIndex < l_Lifetime->FirstUse)
        {
            l_Lifetime->FirstUse = orderIndex;
        }

        if (orderIndex > l_Lifetime->LastUse)
        {
            l_Lifetime->LastUse = orderIndex;
        }

        if (writer)
        {
            if (orderIndex < l_Lifetime->FirstWriter)
            {
                l_Lifetime->FirstWriter = orderIndex;
            }

            if (orderIndex > l_Lifetime->LastWriter)
            {
                l_Lifetime->LastWriter = orderIndex;
            }
        }
    }

    bool RenderGraph::IsResourceMarkedOutput(RenderGraphResourceHandle handle) const
    {
        if (!IsValidHandle(handle))
        {
            return false;
        }

        if (handle.IsTexture())
        {
            return m_TextureDescriptions[handle.Index].ExternalOutput;
        }

        if (handle.IsBuffer())
        {
            return m_BufferDescriptions[handle.Index].ExternalOutput;
        }

        return false;
    }

    const char* RenderGraph::AccessToString(RenderGraphAccess access)
    {
        switch (access)
        {
            case RenderGraphAccess::None:
                return "None";
            case RenderGraphAccess::ColorAttachmentRead:
                return "ColorAttachmentRead";
            case RenderGraphAccess::ColorAttachmentWrite:
                return "ColorAttachmentWrite";
            case RenderGraphAccess::DepthStencilRead:
                return "DepthStencilRead";
            case RenderGraphAccess::DepthStencilWrite:
                return "DepthStencilWrite";
            case RenderGraphAccess::ShaderSampledRead:
                return "ShaderSampledRead";
            case RenderGraphAccess::ShaderStorageRead:
                return "ShaderStorageRead";
            case RenderGraphAccess::ShaderStorageWrite:
                return "ShaderStorageWrite";
            case RenderGraphAccess::ShaderStorageReadWrite:
                return "ShaderStorageReadWrite";
            case RenderGraphAccess::TransferRead:
                return "TransferRead";
            case RenderGraphAccess::TransferWrite:
                return "TransferWrite";
            case RenderGraphAccess::Present:
                return "Present";
            case RenderGraphAccess::VertexBufferRead:
                return "VertexBufferRead";
            case RenderGraphAccess::IndexBufferRead:
                return "IndexBufferRead";
            case RenderGraphAccess::UniformBufferRead:
                return "UniformBufferRead";
            case RenderGraphAccess::StorageBufferRead:
                return "StorageBufferRead";
            case RenderGraphAccess::StorageBufferWrite:
                return "StorageBufferWrite";
            case RenderGraphAccess::StorageBufferReadWrite:
                return "StorageBufferReadWrite";
            case RenderGraphAccess::IndirectCommandRead:
                return "IndirectCommandRead";
        }

        return "Unknown";
    }

    const char* RenderGraph::PassTypeToString(RenderGraphPassType type)
    {
        switch (type)
        {
            case RenderGraphPassType::Graphics:
                return "Graphics";
            case RenderGraphPassType::Compute:
                return "Compute";
            case RenderGraphPassType::Transfer:
                return "Transfer";
            case RenderGraphPassType::Present:
                return "Present";
        }

        return "Unknown";
    }
}