#pragma once

#include "Trinity/Renderer/RenderGraph/RenderGraph.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace Trinity
{
    class VulkanRendererAPI;

    class VulkanRenderGraph final : public RenderGraph
    {
    public:
        explicit VulkanRenderGraph(VulkanRendererAPI& renderer);
        ~VulkanRenderGraph() override = default;

    protected:
        std::vector<std::shared_ptr<Texture>> AllocateTextureBatch(const std::vector<RenderGraphTextureDescription>& descriptions, const std::vector<RenderGraphResourceLifetime>& lifetimes) override;
        std::shared_ptr<Buffer> AllocateBuffer(const RenderGraphBufferDescription& description) override;

        void OnReset() override;
        void OnCompile() override;
        void OnExecutePassBegin(uint32_t passIndex, RenderGraphContext& context) override;
        void OnExecutePassEnd(uint32_t passIndex, RenderGraphContext& context) override;

    private:
        struct ImageState
        {
            VkImageLayout Layout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkPipelineStageFlags2 Stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            VkAccessFlags2 Access = 0;
        };

        struct BufferState
        {
            VkPipelineStageFlags2 Stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            VkAccessFlags2 Access = 0;
        };

        struct PassBarriers
        {
            std::vector<VkImageMemoryBarrier2> ImageBarriers;
            std::vector<VkBufferMemoryBarrier2> BufferBarriers;
        };

        ImageState GetInitialImageState(RenderGraphAccess access, TextureFormat format) const;
        BufferState GetInitialBufferState(RenderGraphAccess access) const;

        ImageState GetImageState(RenderGraphAccess access, TextureFormat format) const;
        BufferState GetBufferState(RenderGraphAccess access) const;

        void AppendTextureBarrierIfNeeded(std::vector<VkImageMemoryBarrier2>& out, uint32_t textureIndex, const ImageState& current, const ImageState& next);
        void AppendBufferBarrierIfNeeded(std::vector<VkBufferMemoryBarrier2>& out, uint32_t bufferIndex, const BufferState& current, const BufferState& next);
        void ProcessTextureUsage(uint32_t passIndex, uint32_t textureIndex, RenderGraphAccess access, std::vector<ImageState>& currentStates);
        void ProcessBufferUsage(uint32_t passIndex, uint32_t bufferIndex, RenderGraphAccess access, std::vector<BufferState>& currentStates);

    private:
        VulkanRendererAPI* m_Renderer = nullptr;

        std::vector<PassBarriers> m_PassBarriers;

        std::vector<bool> m_AliasedTextures;
        std::vector<bool> m_DebugLabelOpen;
    };
}