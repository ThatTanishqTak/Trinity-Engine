#pragma once

#include <cstdint>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Renderer/RHI/Handle.h>

namespace Trinity
{
    struct Viewport
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Width = 0.0f;
        float Height = 0.0f;
        float MinDepth = 0.0f;
        float MaxDepth = 1.0f;
    };

    struct Scissor
    {
        int32_t X = 0;
        int32_t Y = 0;
        
        uint32_t Width = 0;
        uint32_t Height = 0;
    };

    struct RenderingAttachment
    {
        TextureHandle Target;
        
        bool Clear = true;
        
        float ClearColor[4] = { 0.05f, 0.05f, 0.05f, 1.0f };
    };

    struct DepthAttachment
    {
        TextureHandle Target;
        
        bool Clear = true;
        
        float ClearDepth = 1.0f;
    };

    struct RenderingInfo
    {
        const RenderingAttachment* ColorAttachments = nullptr;
        const DepthAttachment* Depth = nullptr;
        
        uint32_t ColorAttachmentCount = 0;
        uint32_t Width = 0;
        uint32_t Height = 0;
    };

    class CommandList
    {
    public:
        virtual ~CommandList() = default;

        virtual void Begin() = 0;
        virtual void End() = 0;

        virtual void BeginRendering(const RenderingInfo& info) = 0;
        virtual void EndRendering() = 0;

        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const Scissor& scissor) = 0;

        virtual void BindPipeline(PipelineHandle pipeline) = 0;
        virtual void BindVertexBuffer(BufferHandle buffer, uint64_t offset = 0) = 0;
        virtual void BindIndexBuffer(BufferHandle buffer, uint64_t offset = 0) = 0;

        virtual void PushConstants(ShaderStage stages, uint32_t offset, uint32_t size, const void* data) = 0;

        virtual void BindTexture(uint32_t set, uint32_t binding, TextureHandle texture, SamplerHandle sampler) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstCount, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;

        virtual void TransitionTexture(TextureHandle texture, ResourceState from, ResourceState to) = 0;
    };
}