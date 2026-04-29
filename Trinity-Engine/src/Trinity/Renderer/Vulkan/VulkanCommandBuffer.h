#pragma once

#include "Trinity/Renderer/CommandBuffer.h"

#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanCommandBuffer : CommandBuffer
    {
    public:
        void BindPipeline() override;
        void BindVertexBuffer(uint32_t binding, Buffer& buffer, uint64_t offset = 0) override;
        void BindIndexBuffer(Buffer& buffer, IndexType type, uint64_t offset = 0) override;
        void BindTexture(uint32_t set, uint32_t binding, Texture& texture, Sampler& sampler) override;
        void BindUniformBuffer(uint32_t set, uint32_t binding, Buffer& buffer, uint64_t offset, uint64_t range) override;

        void PushConstants(ShaderStageFlags stages, uint32_t offset, uint32_t size, const void* data) override;

        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const ScissorRect& scissor) override;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;

    private:
        VkCommandBuffer m_CommandBuffer;
        VkPipeline m_Pipeline;
    };
}