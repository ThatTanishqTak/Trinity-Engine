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

    std::vector<std::shared_ptr<Texture>> VulkanRenderGraph::AllocateResourceBatch(const std::vector<RenderGraphTextureDescription>& descriptions, const std::vector<ResourceLifetime>& lifetimes)
    {
        const size_t l_Count = descriptions.size();
        std::vector<std::shared_ptr<Texture>> l_Output(l_Count);
        m_AliasedResources.assign(l_Count, false);

        if (l_Count == 0)
        {
            return l_Output;
        }

        std::vector<VulkanTransientPool::AllocationRequest> l_TransientRequests;
        std::vector<uint32_t> l_TransientOriginalIndex;
        l_TransientRequests.reserve(l_Count);
        l_TransientOriginalIndex.reserve(l_Count);

        for (size_t i = 0; i < l_Count; ++i)
        {
            const auto& a_Desc = descriptions[i];

            const uint32_t l_EffWidth = a_Desc.Width > 0 ? a_Desc.Width : m_SwapchainWidth;
            const uint32_t l_EffHeight = a_Desc.Height > 0 ? a_Desc.Height : m_SwapchainHeight;

            if (l_EffWidth == 0 || l_EffHeight == 0)
            {
                TR_CORE_ERROR("VulkanRenderGraph: resource '{}' has zero size and no swapchain fallback available", a_Desc.DebugName);
                continue;
            }

            if (a_Desc.Persistent || !lifetimes[i].IsValid())
            {
                TextureSpecification l_Spec{};
                l_Spec.Width = l_EffWidth;
                l_Spec.Height = l_EffHeight;
                l_Spec.MipLevels = a_Desc.MipLevels > 0 ? a_Desc.MipLevels : 1;
                l_Spec.ArrayLayers = a_Desc.ArrayLayers > 0 ? a_Desc.ArrayLayers : 1;
                l_Spec.Format = a_Desc.Format;
                l_Spec.Usage = a_Desc.Usage;
                l_Spec.DebugName = a_Desc.DebugName;

                l_Output[i] = std::make_shared<VulkanTexture>(m_Renderer->GetDevice().GetDevice(), m_Renderer->GetAllocator().GetAllocator(), l_Spec);
                continue;
            }

            VulkanTransientPool::AllocationRequest l_Request{};
            l_Request.Spec.Width = l_EffWidth;
            l_Request.Spec.Height = l_EffHeight;
            l_Request.Spec.MipLevels = a_Desc.MipLevels > 0 ? a_Desc.MipLevels : 1;
            l_Request.Spec.ArrayLayers = a_Desc.ArrayLayers > 0 ? a_Desc.ArrayLayers : 1;
            l_Request.Spec.Format = a_Desc.Format;
            l_Request.Spec.Usage = a_Desc.Usage;
            l_Request.Spec.DebugName = a_Desc.DebugName;
            l_Request.FirstUse = lifetimes[i].FirstUse;
            l_Request.LastUse = lifetimes[i].LastUse;

            l_TransientRequests.push_back(l_Request);
            l_TransientOriginalIndex.push_back(static_cast<uint32_t>(i));
        }

        if (l_TransientRequests.empty())
        {
            return l_Output;
        }

        auto l_Allocations = m_Renderer->GetTransientPool().AllocateBatch(l_TransientRequests);

        for (size_t j = 0; j < l_Allocations.size(); ++j)
        {
            const uint32_t l_OrigIndex = l_TransientOriginalIndex[j];
            l_Output[l_OrigIndex] = l_Allocations[j].Texture;
            m_AliasedResources[l_OrigIndex] = l_Allocations[j].AliasedReuse;
        }

        const VulkanTransientPool& l_Pool = m_Renderer->GetTransientPool();
        TR_CORE_DEBUG("VulkanRenderGraph: allocated {} transient + {} persistent resources (TransientSum = {} KB, TransientPeak = {} KB, Savings = {} KB, Blocks = {})", l_Allocations.size(), l_Count - l_Allocations.size(), l_Pool.GetLastBatchSumBytes() / 1024, l_Pool.GetLastBatchUsedBytes() / 1024, l_Pool.GetLastAliasingSavingsBytes() / 1024, l_Pool.GetBlockCount());

        return l_Output;
    }

    void VulkanRenderGraph::OnReset()
    {
        m_PassBarriers.clear();
        m_AliasedResources.clear();
    }

    void VulkanRenderGraph::OnCompile()
    {
        m_PassBarriers.assign(m_Passes.size(), {});

        const uint32_t l_ResourceCount = static_cast<uint32_t>(m_ResourceDescription.size());
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

                const ResourceState l_Needed = GetReadState(m_ResourceDescription[it_Handle.Index]);
                AppendBarrierIfNeeded(a_OutBarriers, it_Handle.Index, l_Current[it_Handle.Index], l_Needed);

                l_Current[it_Handle.Index] = l_Needed;
            }

            for (const auto& it_Handle : a_Pass.GetWrites())
            {
                if (!it_Handle.IsValid() || it_Handle.Index >= l_ResourceCount)
                {
                    continue;
                }

                const ResourceState l_Needed = GetWriteState(m_ResourceDescription[it_Handle.Index]);
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
        l_Barrier.subresourceRange.aspectMask = VulkanUtilities::GetAspectFlags(m_ResourceDescription[resourceIndex].Format);
        l_Barrier.subresourceRange.baseMipLevel = 0;
        l_Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        l_Barrier.subresourceRange.baseArrayLayer = 0;
        l_Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        if (current.Layout == VK_IMAGE_LAYOUT_UNDEFINED && resourceIndex < m_AliasedResources.size() && m_AliasedResources[resourceIndex])
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            l_Barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        }

        out.push_back(l_Barrier);
    }
}