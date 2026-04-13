#pragma once

#include "Trinity/Renderer/RenderGraph/RenderGraph.h"

#include <vulkan/vulkan.h>

#include <cstdint>
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
        std::shared_ptr<Texture> CreateResource(const RenderGraphTextureDescription& description) override;
        void OnReset() override;
        void OnCompile() override;
        void OnExecutePassBegin(uint32_t passIndex, RenderGraphContext& context) override;
        void OnExecutePassEnd(uint32_t passIndex, RenderGraphContext& context) override;
    
    private:
        struct ResourceState
        {
            VkImageLayout Layout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkPipelineStageFlags2 Stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            VkAccessFlags2 Access = 0;
        };

        struct PassBarriers
        {
            std::vector<VkImageMemoryBarrier2> Barriers;
        };

        ResourceState GetReadState(const RenderGraphTextureDescription& description) const;
        ResourceState GetWriteState(const RenderGraphTextureDescription& description) const;

        void AppendBarrierIfNeeded(std::vector<VkImageMemoryBarrier2>& out, uint32_t resourceIndex, const ResourceState& current, const ResourceState& next);

    private:
        VulkanRendererAPI* m_Renderer = nullptr;

        std::vector<PassBarriers> m_PassBarriers;
    };
}