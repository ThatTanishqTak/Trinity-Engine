#pragma once

#include <vulkan/vulkan.h>

#include <Trinity/Renderer/RHI/CommandList.h>

namespace Trinity
{
    class VulkanDevice;

    class VulkanCommandList : public CommandList
    {
    public:
        explicit VulkanCommandList(VulkanDevice& device);
        ~VulkanCommandList() override;

        VulkanCommandList(const VulkanCommandList&) = delete;
        VulkanCommandList& operator=(const VulkanCommandList&) = delete;

        void Begin() override;
        void End() override;

        void BeginRendering(const RenderingInfo& renderingInfo) override;
        void EndRendering() override;

        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Scissor& scissor) override;

        void BindPipeline(PipelineHandle pipeline) override;
        void BindVertexBuffer(BufferHandle buffer, uint64_t offset = 0) override;
        void BindIndexBuffer(BufferHandle buffer, uint64_t offset = 0) override;

        void PushConstants(ShaderStage stages, uint32_t offset, uint32_t size, const void* data) override;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;

        void TransitionTexture(TextureHandle texture, ResourceState from, ResourceState to) override;

        VkCommandBuffer GetHandle() const { return m_CommandBuffer; }

    private:
        VulkanDevice& m_Device;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        VkPipelineLayout m_CurrentLayout = VK_NULL_HANDLE;
    };
}