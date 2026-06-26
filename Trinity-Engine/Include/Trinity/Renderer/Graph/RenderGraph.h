#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <vector>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Renderer/RHI/Handle.h>

namespace Trinity
{
    class CommandList;

    using RenderGraphExecute = std::function<void(CommandList&)>;

    struct RenderGraphColorTarget
    {
        TextureHandle Target;

        bool Clear = false;
        float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    struct RenderGraphPass
    {
        std::string Name;

        // Sampled inputs transitioned to ShaderResource before the pass runs.
        std::vector<TextureHandle> Reads;

        // Color/depth outputs transitioned to RenderTarget/DepthStencil before the pass runs.
        std::vector<RenderGraphColorTarget> Colors;
        TextureHandle Depth;
        bool ClearDepth = true;
        float DepthClearValue = 1.0f;
        bool ManageRendering = true;

        uint32_t Width = 0;
        uint32_t Height = 0;

        RenderGraphExecute Execute;
    };

    class RenderGraph
    {
    public:
        struct PassInfo
        {
            std::string Name;
            std::vector<std::string> Reads;
            std::vector<std::string> Writes;
            bool Managed = true;
        };

        // Start a new frame: clears passes and resource state tracking.
        void Reset();

        // Register an external resource and its state at the start of the frame.
        void Import(TextureHandle handle, ResourceState initialState, const std::string& name = "");

        // Add a pass. The reference is stable until the next Reset().
        RenderGraphPass& AddPass(const std::string& name);

        // Resource transitioned to Present after all passes have executed.
        void SetPresent(TextureHandle handle);

        // Record every pass into the command list (call between Begin and End).
        void Execute(CommandList& commandList);

        // Last-executed pass list, for debugging and editor inspection.
        const std::vector<PassInfo>& GetPasses() const { return m_PassInfo; }

    private:
        struct ResourceEntry
        {
            TextureHandle Handle;
            ResourceState State = ResourceState::Undefined;
        };

        struct ResourceName
        {
            TextureHandle Handle;
            std::string Name;
        };

        ResourceState GetState(TextureHandle handle) const;
        void SetState(TextureHandle handle, ResourceState state);
        void Transition(CommandList& commandList, TextureHandle handle, ResourceState target);
        std::string NameOf(TextureHandle handle) const;

    private:
        std::deque<RenderGraphPass> m_Passes;
        std::vector<ResourceEntry> m_States;
        std::vector<ResourceName> m_Names;
        std::vector<PassInfo> m_PassInfo;

        TextureHandle m_Present;
        bool m_HasPresent = false;
    };
}