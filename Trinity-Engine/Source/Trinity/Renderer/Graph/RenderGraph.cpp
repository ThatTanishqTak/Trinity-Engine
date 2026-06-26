#include <Trinity/Renderer/Graph/RenderGraph.h>

#include <utility>

#include <Trinity/Renderer/RHI/CommandList.h>

namespace Trinity
{
    void RenderGraph::Reset()
    {
        m_Passes.clear();
        m_States.clear();
        m_Names.clear();
        m_PassInfo.clear();
        m_Present = TextureHandle{};
        m_HasPresent = false;
    }

    void RenderGraph::Import(TextureHandle handle, ResourceState initialState, const std::string& name)
    {
        if (!handle.IsValid())
        {
            return;
        }

        SetState(handle, initialState);

        if (!name.empty())
        {
            m_Names.push_back({ handle, name });
        }
    }

    RenderGraphPass& RenderGraph::AddPass(const std::string& name)
    {
        m_Passes.emplace_back();
        m_Passes.back().Name = name;

        return m_Passes.back();
    }

    void RenderGraph::SetPresent(TextureHandle handle)
    {
        m_Present = handle;
        m_HasPresent = handle.IsValid();
    }

    ResourceState RenderGraph::GetState(TextureHandle handle) const
    {
        for (const ResourceEntry& it_Entry : m_States)
        {
            if (it_Entry.Handle == handle)
            {
                return it_Entry.State;
            }
        }

        return ResourceState::Undefined;
    }

    void RenderGraph::SetState(TextureHandle handle, ResourceState state)
    {
        for (ResourceEntry& it_Entry : m_States)
        {
            if (it_Entry.Handle == handle)
            {
                it_Entry.State = state;

                return;
            }
        }

        m_States.push_back({ handle, state });
    }

    void RenderGraph::Transition(CommandList& commandList, TextureHandle handle, ResourceState target)
    {
        if (!handle.IsValid())
        {
            return;
        }

        ResourceState l_Current = GetState(handle);
        if (l_Current == target)
        {
            return;
        }

        commandList.TransitionTexture(handle, l_Current, target);
        SetState(handle, target);
    }

    std::string RenderGraph::NameOf(TextureHandle handle) const
    {
        for (const ResourceName& it_Name : m_Names)
        {
            if (it_Name.Handle == handle)
            {
                return it_Name.Name;
            }
        }

        return "Texture#" + std::to_string(handle.GetIndex());
    }

    void RenderGraph::Execute(CommandList& commandList)
    {
        m_PassInfo.clear();

        for (RenderGraphPass& it_Pass : m_Passes)
        {
            PassInfo l_Info;
            l_Info.Name = it_Pass.Name;
            l_Info.Managed = it_Pass.ManageRendering;

            for (TextureHandle it_Read : it_Pass.Reads)
            {
                Transition(commandList, it_Read, ResourceState::ShaderResource);
                l_Info.Reads.push_back(NameOf(it_Read));
            }

            for (const RenderGraphColorTarget& it_Color : it_Pass.Colors)
            {
                Transition(commandList, it_Color.Target, ResourceState::RenderTarget);
                l_Info.Writes.push_back(NameOf(it_Color.Target));
            }

            if (it_Pass.Depth.IsValid())
            {
                Transition(commandList, it_Pass.Depth, ResourceState::DepthStencil);
                l_Info.Writes.push_back(NameOf(it_Pass.Depth));
            }

            if (it_Pass.ManageRendering)
            {
                std::vector<RenderingAttachment> l_ColorAttachments;
                l_ColorAttachments.reserve(it_Pass.Colors.size());
                for (const RenderGraphColorTarget& it_Color : it_Pass.Colors)
                {
                    RenderingAttachment l_Attachment;
                    l_Attachment.Target = it_Color.Target;
                    l_Attachment.Clear = it_Color.Clear;
                    l_Attachment.ClearColor[0] = it_Color.ClearColor[0];
                    l_Attachment.ClearColor[1] = it_Color.ClearColor[1];
                    l_Attachment.ClearColor[2] = it_Color.ClearColor[2];
                    l_Attachment.ClearColor[3] = it_Color.ClearColor[3];
                    l_ColorAttachments.push_back(l_Attachment);
                }

                DepthAttachment l_DepthAttachment;
                bool l_HasDepth = it_Pass.Depth.IsValid();
                if (l_HasDepth)
                {
                    l_DepthAttachment.Target = it_Pass.Depth;
                    l_DepthAttachment.Clear = it_Pass.ClearDepth;
                    l_DepthAttachment.ClearDepth = it_Pass.DepthClearValue;
                }

                RenderingInfo l_RenderingInfo;
                l_RenderingInfo.ColorAttachments = l_ColorAttachments.empty() ? nullptr : l_ColorAttachments.data();
                l_RenderingInfo.ColorAttachmentCount = static_cast<uint32_t>(l_ColorAttachments.size());
                l_RenderingInfo.Depth = l_HasDepth ? &l_DepthAttachment : nullptr;
                l_RenderingInfo.Width = it_Pass.Width;
                l_RenderingInfo.Height = it_Pass.Height;

                commandList.BeginRendering(l_RenderingInfo);

                Viewport l_Viewport;
                l_Viewport.X = 0.0f;
                l_Viewport.Y = 0.0f;
                l_Viewport.Width = static_cast<float>(it_Pass.Width);
                l_Viewport.Height = static_cast<float>(it_Pass.Height);
                l_Viewport.MinDepth = 0.0f;
                l_Viewport.MaxDepth = 1.0f;
                commandList.SetViewport(l_Viewport);

                Scissor l_Scissor;
                l_Scissor.X = 0;
                l_Scissor.Y = 0;
                l_Scissor.Width = it_Pass.Width;
                l_Scissor.Height = it_Pass.Height;
                commandList.SetScissor(l_Scissor);

                if (it_Pass.Execute)
                {
                    it_Pass.Execute(commandList);
                }

                commandList.EndRendering();
            }
            else if (it_Pass.Execute)
            {
                it_Pass.Execute(commandList);
            }

            m_PassInfo.push_back(std::move(l_Info));
        }

        if (m_HasPresent)
        {
            Transition(commandList, m_Present, ResourceState::Present);
        }
    }
}