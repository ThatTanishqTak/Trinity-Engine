#pragma once

#include "Trinity/Renderer/CommandList.h"

#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanPipeline;
    class VulkanComputePipeline;

    class VulkanCommandList final : public CommandList
    {
    public:
        VulkanCommandList() = default;
        ~VulkanCommandList() override;

        VulkanCommandList(const VulkanCommandList&) = delete;
        VulkanCommandList& operator=(const VulkanCommandList&) = delete;

        VulkanCommandList(VulkanCommandList&& other) noexcept;
        VulkanCommandList& operator=(VulkanCommandList&& other) noexcept;

        void Initialize(VkDevice device);
        void Shutdown();

        void Reset(VkCommandBuffer commandBuffer);
        void Invalidate();

        VkCommandBuffer GetHandle() const { return m_CommandBuffer; }
        bool IsValid() const { return m_CommandBuffer != VK_NULL_HANDLE; }

        void Begin() override;
        void End() override;

        void BeginRendering(const RenderingInfo& info) override;
        void EndRendering() override;

        void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) override;
        void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) override;
        void SetDepthBias(float constantFactor, float clamp, float slopeFactor) override;

        void BindPipeline(const std::shared_ptr<Pipeline>& pipeline) override;
        void BindComputePipeline(const std::shared_ptr<ComputePipeline>& pipeline) override;
        void BindDescriptorSet(uint32_t setIndex, const std::shared_ptr<DescriptorSet>& set, uint32_t dynamicOffsetCount = 0, const uint32_t* dynamicOffsets = nullptr) override;
        void PushConstants(uint32_t offset, uint32_t size, const void* data) override;
        void BindVertexBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer, uint64_t offset = 0) override;
        void BindIndexBuffer(const std::shared_ptr<Buffer>& buffer, uint64_t offset = 0, bool indexType32Bit = true) override;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;
        void DrawIndexedIndirect(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset, uint32_t drawCount, uint32_t stride) override;
        void DrawIndexedIndirectCount(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset, const std::shared_ptr<Buffer>& countBuffer, uint64_t countOffset, uint32_t maxDraws, uint32_t stride) override;

        void Dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ) override;
        void DispatchIndirect(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset) override;

        void TransitionResource(const std::shared_ptr<Texture>& texture, ResourceState before, ResourceState after) override;
        void BufferBarrier(const std::shared_ptr<Buffer>& buffer, ResourceState before, ResourceState after) override;

        void CopyBuffer(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Buffer>& dst, uint64_t srcOffset, uint64_t dstOffset, uint64_t size) override;
        void CopyBufferToTexture(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Texture>& dst, uint32_t mipLevel, uint32_t arrayLayer) override;
        void BlitTexture(const std::shared_ptr<Texture>& src, const std::shared_ptr<Texture>& dst) override;
        void GenerateMips(const std::shared_ptr<Texture>& texture) override;

        void WriteTimestamp(const std::shared_ptr<QueryPool>& pool, uint32_t index) override;
        void ResetQueryPool(const std::shared_ptr<QueryPool>& pool, uint32_t firstIndex, uint32_t count) override;

        void BeginDebugLabel(const std::string& name, const float color[4]) override;
        void EndDebugLabel() override;
        void InsertDebugMarker(const std::string& name) override;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;

        VulkanPipeline* m_BoundGraphicsPipeline = nullptr;
        VulkanComputePipeline* m_BoundComputePipeline = nullptr;
        VkPipelineLayout m_BoundLayout = VK_NULL_HANDLE;
    };
}