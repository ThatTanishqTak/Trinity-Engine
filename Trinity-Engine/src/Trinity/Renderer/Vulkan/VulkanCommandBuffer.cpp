#include "Trinity/Renderer/Vulkan/VulkanCommandBuffer.h"

#include <vulkan/vulkan.h>

namespace Trinity
{
    void VulkanCommandBuffer::BindPipeline()
    {
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    }

    void VulkanCommandBuffer::BindVertexBuffer(uint32_t binding, Buffer& buffer, uint64_t offset)
    {

    }

    void VulkanCommandBuffer::BindIndexBuffer(Buffer& buffer, IndexType type, uint64_t offset)
    {

    }

    void VulkanCommandBuffer::BindTexture(uint32_t set, uint32_t binding, Texture& texture, Sampler& sampler)
    {

    }

    void VulkanCommandBuffer::BindUniformBuffer(uint32_t set, uint32_t binding, Buffer& buffer, uint64_t offset, uint64_t range)
    {

    }

    void VulkanCommandBuffer::PushConstants(ShaderStageFlags stages, uint32_t offset, uint32_t size, const void* data)
    {

    }

    void VulkanCommandBuffer::SetViewport(const Viewport& viewport)
    {

    }

    void VulkanCommandBuffer::SetScissor(const ScissorRect& scissor)
    {

    }

    void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {

    }

    void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {

    }
}