#include "Trinity/Renderer/Vulkan/RenderGraph/VulkanRenderGraph.h"

#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"

namespace Trinity
{
    VulkanRenderGraph::VulkanRenderGraph(VulkanRendererAPI& renderer) : m_Renderer(&renderer)
    {
        m_SwapchainWidth = renderer.GetSwapchainWidth();
        m_SwapchainHeight = renderer.GetSwapchainHeight();
    }

    std::shared_ptr<Texture> VulkanRenderGraph::CreateResource(const RenderGraphTextureDescription& description)
    {
        TextureSpecification l_Specification{};
        l_Specification.Width = description.Width;
        l_Specification.Height = description.Height;
        l_Specification.MipLevels = 1;
        l_Specification.Format = description.Format;
        l_Specification.Usage = description.Usage;
        l_Specification.DebugName = description.DebugName;

        return std::make_shared<VulkanTexture>(m_Renderer->GetDevice().GetDevice(), m_Renderer->GetAllocator().GetAllocator(), l_Specification);
    }

    void VulkanRenderGraph::OnReset()
    {
        m_PassBarriers.clear();
    }

    void VulkanRenderGraph::OnCompile()
    {
        m_PassBarriers.assign(m_Passes.size(), {});

        const uint32_t l_ResourceCount = static_cast<uint32_t>(m_ResourceDescs.size());
        std::vector<ResourceState> l_Current(l_ResourceCount);
        for (uint32_t it_OrderIndex : m_ExecutionOrder)
        {
            if (it_OrderIndex >= m_Passes.size())
            {
                continue;
            }

            const auto& a_Pass = m_Passes[it_OrderIndex];
            auto& a_OutBarriers = m_PassBarriers[it_OrderIndex].Barriers;

            for (const auto& it_Handle : a_Pass.GetReads())
            {
                if (!it_Handle.IsValid() || it_Handle.Index >= l_ResourceCount)
                {
                    continue;
                }

                const ResourceState l_Needed = GetReadState(m_ResourceDescs[it_Handle.Index]);
                AppendBarrierIfNeeded(a_OutBarriers, it_Handle.Index, l_Current[it_Handle.Index], l_Needed);
                l_Current[it_Handle.Index] = l_Needed;
            }
            for (const auto& it_Handle : a_Pass.GetWrites())
            {
                if (!it_Handle.IsValid() || it_Handle.Index >= l_ResourceCount)
                {
                    continue;
                }

                const ResourceState l_Needed = GetWriteState(m_ResourceDescs[it_Handle.Index]);
                AppendBarrierIfNeeded(a_OutBarriers, it_Handle.Index, l_Current[it_Handle.Index], l_Needed);
                l_Current[it_Handle.Index] = l_Needed;
            }
        }
    }
    void VulkanRenderGraph::OnExecutePassBegin(uint32_t passIndex, RenderGraphContext& context)
    {
        (void)context;

        if (passIndex >= m_PassBarriers.size() || m_PassBarriers[passIndex].Barriers.empty())
        {
            return;
        }

        VkCommandBuffer l_Cmd = m_Renderer->GetCurrentCommandBuffer();
        if (l_Cmd == VK_NULL_HANDLE)
        {
            return;
        }

        VkDependencyInfo l_DependencyInfo{};
        l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_DependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(m_PassBarriers[passIndex].Barriers.size());
        l_DependencyInfo.pImageMemoryBarriers = m_PassBarriers[passIndex].Barriers.data();
        vkCmdPipelineBarrier2(l_Cmd, &l_DependencyInfo);
    }

    void VulkanRenderGraph::OnExecutePassEnd(uint32_t passIndex, RenderGraphContext& context)
    {
        (void)passIndex;
        (void)context;
    }

    VulkanRenderGraph::ResourceState VulkanRenderGraph::GetReadState(const RenderGraphTextureDescription& description) const
    {
        ResourceState l_State{};
        if (VulkanUtilities::IsDepthFormat(description.Format))
        {
            l_State.Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            l_State.Stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            l_State.Access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        }
        else if (description.Usage & TextureUsage::Storage)
        {
            l_State.Layout = VK_IMAGE_LAYOUT_GENERAL;
            l_State.Stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            l_State.Access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        }
        else
        {
            l_State.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            l_State.Stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            l_State.Access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        }

        return l_State;
    }

    VulkanRenderGraph::ResourceState VulkanRenderGraph::GetWriteState(const RenderGraphTextureDescription& description) const
    {
        ResourceState l_State{};
        if (VulkanUtilities::IsDepthFormat(description.Format))
        {
            l_State.Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            l_State.Stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            l_State.Access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }
        else if (description.Usage & TextureUsage::Storage)
        {
            l_State.Layout = VK_IMAGE_LAYOUT_GENERAL;
            l_State.Stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            l_State.Access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
        }
        else
        {
            l_State.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            l_State.Stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            l_State.Access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        }

        return l_State;
    }

    void VulkanRenderGraph::AppendBarrierIfNeeded(std::vector<VkImageMemoryBarrier2>& out, uint32_t resourceIndex, const ResourceState& current, const ResourceState& next)
    {
        if (resourceIndex >= m_Resources.size())
        {
            return;
        }

        auto a_Texture = std::dynamic_pointer_cast<VulkanTexture>(m_Resources[resourceIndex]);
        if (!a_Texture)
        {
            return;
        }

        if (current.Layout == next.Layout && current.Stage == next.Stage && current.Access == next.Access)
        {
            return;
        }

        VkImageMemoryBarrier2 l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        l_Barrier.srcStageMask = current.Stage;
        l_Barrier.srcAccessMask = current.Access;
        l_Barrier.dstStageMask = next.Stage;
        l_Barrier.dstAccessMask = next.Access;
        l_Barrier.oldLayout = current.Layout;
        l_Barrier.newLayout = next.Layout;
        l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.image = a_Texture->GetImage();
        l_Barrier.subresourceRange.aspectMask = VulkanUtilities::GetAspectFlags(m_ResourceDescs[resourceIndex].Format);
        l_Barrier.subresourceRange.baseMipLevel = 0;
        l_Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        l_Barrier.subresourceRange.baseArrayLayer = 0;
        l_Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        out.push_back(l_Barrier);
    }
}