#pragma once

#include "Trinity/Renderer/Resources/Buffer.h"
#include "Trinity/Renderer/Resources/ComputePipeline.h"
#include "Trinity/Renderer/Resources/DescriptorSet.h"
#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/QueryPool.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Trinity
{
    struct RenderingAttachment
    {
        std::shared_ptr<Texture> ColorTexture;
        bool ClearOnLoad = true;
        float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    struct DepthAttachment
    {
        std::shared_ptr<Texture> DepthTexture;
        bool ClearOnLoad = true;
        float ClearDepth = 0.0f;
        uint32_t ClearStencil = 0;
    };

    struct RenderingInfo
    {
        std::vector<RenderingAttachment> ColorAttachments;
        DepthAttachment Depth;
        uint32_t Width = 0;
        uint32_t Height = 0;
    };

    enum class ResourceState : uint8_t
    {
        Undefined = 0,
        ColorAttachment,
        DepthStencilAttachment,
        DepthStencilRead,
        ShaderRead,
        ShaderWrite,
        TransferSrc,
        TransferDst,
        Present
    };

    class CommandList
    {
    public:
        virtual ~CommandList() = default;

        virtual void Begin() = 0;
        virtual void End() = 0;

        virtual void BeginRendering(const RenderingInfo& info) = 0;
        virtual void EndRendering() = 0;

        virtual void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) = 0;
        virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;

        virtual void BindPipeline(const std::shared_ptr<Pipeline>& pipeline) = 0;
        virtual void BindComputePipeline(const std::shared_ptr<ComputePipeline>& pipeline) = 0;
        virtual void BindDescriptorSet(uint32_t setIndex, const std::shared_ptr<DescriptorSet>& set, uint32_t dynamicOffsetCount = 0, const uint32_t* dynamicOffsets = nullptr) = 0;
        virtual void PushConstants(uint32_t offset, uint32_t size, const void* data) = 0;
        virtual void BindVertexBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer, uint64_t offset = 0) = 0;
        virtual void BindIndexBuffer(const std::shared_ptr<Buffer>& buffer, uint64_t offset = 0, bool indexType32Bit = true) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void DrawIndexedIndirect(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawIndexedIndirectCount(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset, const std::shared_ptr<Buffer>& countBuffer, uint64_t countOffset, uint32_t maxDraws, uint32_t stride) = 0;

        virtual void Dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = 0;
        virtual void DispatchIndirect(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset) = 0;

        virtual void TransitionResource(const std::shared_ptr<Texture>& texture, ResourceState before, ResourceState after) = 0;
        virtual void BufferBarrier(const std::shared_ptr<Buffer>& buffer, ResourceState before, ResourceState after) = 0;

        virtual void CopyBuffer(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Buffer>& dst, uint64_t srcOffset, uint64_t dstOffset, uint64_t size) = 0;
        virtual void CopyBufferToTexture(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Texture>& dst, uint32_t mipLevel, uint32_t arrayLayer) = 0;
        virtual void BlitTexture(const std::shared_ptr<Texture>& src, const std::shared_ptr<Texture>& dst) = 0;
        virtual void GenerateMips(const std::shared_ptr<Texture>& texture) = 0;

        virtual void WriteTimestamp(const std::shared_ptr<QueryPool>& pool, uint32_t index) = 0;
        virtual void ResetQueryPool(const std::shared_ptr<QueryPool>& pool, uint32_t firstIndex, uint32_t count) = 0;

        virtual void BeginDebugLabel(const std::string& name, const float color[4]) = 0;
        virtual void EndDebugLabel() = 0;
        virtual void InsertDebugMarker(const std::string& name) = 0;
    };
}