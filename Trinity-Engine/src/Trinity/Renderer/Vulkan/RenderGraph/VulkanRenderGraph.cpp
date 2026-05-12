#include "Trinity/Renderer/Vulkan/RenderGraph/VulkanRenderGraph.h"

#include "Trinity/Renderer/Vulkan/VulkanRendererAPI.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Utilities/Log.h"

#include <algorithm>

namespace Trinity
{
    VulkanRenderGraph::VulkanRenderGraph(VulkanRendererAPI& renderer) : m_Renderer(&renderer)
    {
        m_SwapchainWidth = renderer.GetSwapchainWidth();
        m_SwapchainHeight = renderer.GetSwapchainHeight();
    }

    std::vector<std::shared_ptr<Texture>> VulkanRenderGraph::AllocateTextureBatch(const std::vector<RenderGraphTextureDescription>& descriptions, const std::vector<RenderGraphResourceLifetime>& lifetimes)
    {
        const size_t l_Count = descriptions.size();

        std::vector<std::shared_ptr<Texture>> l_Output(l_Count);
        m_AliasedTextures.assign(l_Count, false);

        if (l_Count == 0)
        {
            return l_Output;
        }

        std::vector<VulkanTransientPool::AllocationRequest> l_TransientRequests;
        std::vector<uint32_t> l_TransientOriginalIndex;

        l_TransientRequests.reserve(l_Count);
        l_TransientOriginalIndex.reserve(l_Count);

        for (uint32_t i = 0; i < static_cast<uint32_t>(l_Count); ++i)
        {
            const RenderGraphTextureDescription& l_Description = descriptions[i];

            const bool l_UseSwapchainSize = l_Description.MatchSwapchainSize || l_Description.Width == 0 || l_Description.Height == 0;

            const uint32_t l_EffectiveWidth = l_UseSwapchainSize ? m_SwapchainWidth : l_Description.Width;
            const uint32_t l_EffectiveHeight = l_UseSwapchainSize ? m_SwapchainHeight : l_Description.Height;

            if (l_Description.Imported)
            {
                l_Output[i] = m_Textures[i];
                continue;
            }

            if (l_EffectiveWidth == 0 || l_EffectiveHeight == 0)
            {
                TR_CORE_ERROR("VulkanRenderGraph: texture '{}' has zero size and no swapchain fallback available", l_Description.DebugName);

                continue;
            }

            TextureSpecification l_TextureSpecification{};
            l_TextureSpecification.Width = l_EffectiveWidth;
            l_TextureSpecification.Height = l_EffectiveHeight;
            l_TextureSpecification.MipLevels = std::max(1u, l_Description.MipLevels);
            l_TextureSpecification.ArrayLayers = std::max(1u, l_Description.ArrayLayers);
            l_TextureSpecification.Samples = std::max(1u, l_Description.SampleCount);
            l_TextureSpecification.Format = l_Description.Format;
            l_TextureSpecification.Usage = l_Description.Usage;
            l_TextureSpecification.DebugName = l_Description.DebugName;

            const bool l_ShouldAllocatePersistent = l_Description.Persistent || !lifetimes[i].IsValid();

            if (l_ShouldAllocatePersistent)
            {
                l_Output[i] = std::make_shared<VulkanTexture>(m_Renderer->GetDevice().GetDevice(), m_Renderer->GetAllocator().GetAllocator(), l_TextureSpecification);

                continue;
            }

            VulkanTransientPool::AllocationRequest l_Request{};
            l_Request.Spec = l_TextureSpecification;
            l_Request.FirstUse = lifetimes[i].FirstUse;
            l_Request.LastUse = lifetimes[i].LastUse;

            l_TransientRequests.push_back(l_Request);
            l_TransientOriginalIndex.push_back(i);
        }

        if (!l_TransientRequests.empty())
        {
            auto l_Allocations = m_Renderer->GetTransientPool().AllocateBatch(l_TransientRequests);

            for (uint32_t i = 0; i < static_cast<uint32_t>(l_Allocations.size()); ++i)
            {
                const uint32_t l_OriginalIndex = l_TransientOriginalIndex[i];

                l_Output[l_OriginalIndex] = l_Allocations[i].Texture;
                m_AliasedTextures[l_OriginalIndex] = l_Allocations[i].AliasedReuse;
            }

            const VulkanTransientPool& l_Pool = m_Renderer->GetTransientPool();

            TR_CORE_DEBUG("VulkanRenderGraph: allocated {} transient + {} persistent/imported textures (TransientSum = {} KB, TransientPeak = {} KB, Savings = {} KB, Blocks = {})", l_Allocations.size(), l_Count - l_Allocations.size(), l_Pool.GetLastBatchSumBytes() / 1024, l_Pool.GetLastBatchUsedBytes() / 1024, l_Pool.GetLastAliasingSavingsBytes() / 1024, l_Pool.GetBlockCount());
        }

        return l_Output;
    }

    std::shared_ptr<Buffer> VulkanRenderGraph::AllocateBuffer(const RenderGraphBufferDescription& description)
    {
        BufferSpecification l_Specification{};
        l_Specification.Size = description.Size;
        l_Specification.Usage = description.Usage;
        l_Specification.MemoryType = description.MemoryType;
        l_Specification.DebugName = description.DebugName;

        return m_Renderer->CreateBuffer(l_Specification);
    }

    void VulkanRenderGraph::OnReset()
    {
        m_PassBarriers.clear();
        m_AliasedTextures.clear();
        m_DebugLabelOpen.clear();
    }

    void VulkanRenderGraph::OnCompile()
    {
        m_PassBarriers.assign(m_Passes.size(), {});
        m_DebugLabelOpen.assign(m_Passes.size(), false);

        std::vector<ImageState> l_CurrentImageStates(m_TextureDescriptions.size());
        std::vector<BufferState> l_CurrentBufferStates(m_BufferDescriptions.size());

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_TextureDescriptions.size()); ++i)
        {
            if (i < m_TextureInitialAccesses.size() && m_TextureInitialAccesses[i] != RenderGraphAccess::None)
            {
                l_CurrentImageStates[i] = GetInitialImageState(m_TextureInitialAccesses[i], m_TextureDescriptions[i].Format);
            }
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_BufferDescriptions.size()); ++i)
        {
            if (i < m_BufferInitialAccesses.size() && m_BufferInitialAccesses[i] != RenderGraphAccess::None)
            {
                l_CurrentBufferStates[i] = GetInitialBufferState(m_BufferInitialAccesses[i]);
            }
        }

        for (uint32_t it_OrderIndex : m_ExecutionOrder)
        {
            if (it_OrderIndex >= m_Passes.size())
            {
                continue;
            }

            const RenderGraphPass& l_Pass = m_Passes[it_OrderIndex];

            auto ProcessUsage = [&](const RenderGraphResourceUsage& usage)
            {
                if (!usage.Resource.IsValid())
                {
                    return;
                }

                if (usage.Resource.IsTexture())
                {
                    ProcessTextureUsage(it_OrderIndex, usage.Resource.Index, usage.Access, l_CurrentImageStates);
                }
                else if (usage.Resource.IsBuffer())
                {
                    ProcessBufferUsage(it_OrderIndex, usage.Resource.Index, usage.Access, l_CurrentBufferStates);
                }
            };

            for (const RenderGraphResourceUsage& it_Read : l_Pass.GetReads())
            {
                ProcessUsage(it_Read);
            }

            for (const RenderGraphResourceUsage& it_Write : l_Pass.GetWrites())
            {
                ProcessUsage(it_Write);
            }

            for (const RenderGraphResourceUsage& it_ReadWrite : l_Pass.GetReadWrites())
            {
                ProcessUsage(it_ReadWrite);
            }
        }
    }

    void VulkanRenderGraph::OnExecutePassBegin(uint32_t passIndex, RenderGraphContext& context)
    {
        if (passIndex < m_Passes.size())
        {
            const RenderGraphPass& l_Pass = m_Passes[passIndex];

            context.GetCommandList().BeginDebugLabel(l_Pass.GetName(), l_Pass.GetDebugColor().data());
            if (passIndex < m_DebugLabelOpen.size())
            {
                m_DebugLabelOpen[passIndex] = true;
            }
        }

        if (passIndex >= m_PassBarriers.size())
        {
            return;
        }

        const PassBarriers& l_Barriers = m_PassBarriers[passIndex];

        if (l_Barriers.ImageBarriers.empty() && l_Barriers.BufferBarriers.empty())
        {
            return;
        }

        VkCommandBuffer l_CommandBuffer = m_Renderer->GetCurrentCommandBuffer();
        if (l_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VkDependencyInfo l_DependencyInfo{};
        l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;

        l_DependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(l_Barriers.ImageBarriers.size());
        l_DependencyInfo.pImageMemoryBarriers = l_Barriers.ImageBarriers.empty() ? nullptr : l_Barriers.ImageBarriers.data();

        l_DependencyInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(l_Barriers.BufferBarriers.size());
        l_DependencyInfo.pBufferMemoryBarriers = l_Barriers.BufferBarriers.empty() ? nullptr : l_Barriers.BufferBarriers.data();

        vkCmdPipelineBarrier2(l_CommandBuffer, &l_DependencyInfo);
    }

    void VulkanRenderGraph::OnExecutePassEnd(uint32_t passIndex, RenderGraphContext& context)
    {
        if (passIndex < m_DebugLabelOpen.size() && m_DebugLabelOpen[passIndex])
        {
            context.GetCommandList().EndDebugLabel();
            m_DebugLabelOpen[passIndex] = false;
        }
    }

    VulkanRenderGraph::ImageState VulkanRenderGraph::GetInitialImageState(RenderGraphAccess access, TextureFormat format) const
    {
        if (access == RenderGraphAccess::None)
        {
            return {};
        }

        return GetImageState(access, format);
    }

    VulkanRenderGraph::BufferState VulkanRenderGraph::GetInitialBufferState(RenderGraphAccess access) const
    {
        if (access == RenderGraphAccess::None)
        {
            return {};
        }

        return GetBufferState(access);
    }

    VulkanRenderGraph::ImageState VulkanRenderGraph::GetImageState(RenderGraphAccess access, TextureFormat format) const
    {
        ImageState l_State{};

        const bool l_DepthFormat = VulkanUtilities::IsDepthFormat(format);

        switch (access)
        {
            case RenderGraphAccess::ColorAttachmentRead:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                l_State.Stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                l_State.Access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;

                break;
            }

            case RenderGraphAccess::ColorAttachmentWrite:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                l_State.Stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                l_State.Access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

                break;
            }

            case RenderGraphAccess::DepthStencilRead:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                l_State.Stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                l_State.Access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

                break;
            }

            case RenderGraphAccess::DepthStencilWrite:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                l_State.Stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                l_State.Access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                break;
            }

            case RenderGraphAccess::ShaderSampledRead:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                l_State.Stage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                l_State.Access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;

                break;
            }

            case RenderGraphAccess::ShaderStorageRead:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_GENERAL;
                l_State.Stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                l_State.Access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;

                break;
            }

            case RenderGraphAccess::ShaderStorageWrite:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_GENERAL;
                l_State.Stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                l_State.Access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;

                break;
            }

            case RenderGraphAccess::ShaderStorageReadWrite:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_GENERAL;
                l_State.Stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                l_State.Access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;

                break;
            }

            case RenderGraphAccess::TransferRead:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                l_State.Stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                l_State.Access = VK_ACCESS_2_TRANSFER_READ_BIT;

                break;
            }

            case RenderGraphAccess::TransferWrite:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                l_State.Stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                l_State.Access = VK_ACCESS_2_TRANSFER_WRITE_BIT;

                break;
            }

            case RenderGraphAccess::Present:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                l_State.Stage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
                l_State.Access = 0;

                break;
            }

            case RenderGraphAccess::None:
            case RenderGraphAccess::VertexBufferRead:
            case RenderGraphAccess::IndexBufferRead:
            case RenderGraphAccess::UniformBufferRead:
            case RenderGraphAccess::StorageBufferRead:
            case RenderGraphAccess::StorageBufferWrite:
            case RenderGraphAccess::StorageBufferReadWrite:
            case RenderGraphAccess::IndirectCommandRead:
            default:
            {
                l_State.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
                l_State.Stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                l_State.Access = 0;

                break;
            }
        }

        return l_State;
    }

    VulkanRenderGraph::BufferState VulkanRenderGraph::GetBufferState(RenderGraphAccess access) const
    {
        BufferState l_State{};

        switch (access)
        {
            case RenderGraphAccess::VertexBufferRead:
            {
                l_State.Stage = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
                l_State.Access = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;

                break;
            }

            case RenderGraphAccess::IndexBufferRead:
            {
                l_State.Stage = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
                l_State.Access = VK_ACCESS_2_INDEX_READ_BIT;

                break;
            }

            case RenderGraphAccess::UniformBufferRead:
            {
                l_State.Stage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                l_State.Access = VK_ACCESS_2_UNIFORM_READ_BIT;

                break;
            }

            case RenderGraphAccess::StorageBufferRead:
            {
                l_State.Stage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                l_State.Access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;

                break;
            }

            case RenderGraphAccess::StorageBufferWrite:
            {
                l_State.Stage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                l_State.Access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;

                break;
            }

            case RenderGraphAccess::StorageBufferReadWrite:
            {
                l_State.Stage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                l_State.Access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;

                break;
            }

            case RenderGraphAccess::TransferRead:
            {
                l_State.Stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                l_State.Access = VK_ACCESS_2_TRANSFER_READ_BIT;

                break;
            }

            case RenderGraphAccess::TransferWrite:
            {
                l_State.Stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                l_State.Access = VK_ACCESS_2_TRANSFER_WRITE_BIT;

                break;
            }

            case RenderGraphAccess::IndirectCommandRead:
            {
                l_State.Stage = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
                l_State.Access = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;

                break;
            }

            case RenderGraphAccess::None:
            case RenderGraphAccess::ColorAttachmentRead:
            case RenderGraphAccess::ColorAttachmentWrite:
            case RenderGraphAccess::DepthStencilRead:
            case RenderGraphAccess::DepthStencilWrite:
            case RenderGraphAccess::ShaderSampledRead:
            case RenderGraphAccess::ShaderStorageRead:
            case RenderGraphAccess::ShaderStorageWrite:
            case RenderGraphAccess::ShaderStorageReadWrite:
            case RenderGraphAccess::Present:
            default:
            {
                l_State.Stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                l_State.Access = 0;

                break;
            }
        }

        return l_State;
    }

    void VulkanRenderGraph::AppendTextureBarrierIfNeeded(std::vector<VkImageMemoryBarrier2>& out, uint32_t textureIndex, const ImageState& current, const ImageState& next)
    {
        if (textureIndex >= m_Textures.size())
        {
            return;
        }

        auto l_Texture = std::dynamic_pointer_cast<VulkanTexture>(m_Textures[textureIndex]);
        if (!l_Texture)
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

        l_Barrier.image = l_Texture->GetImage();

        l_Barrier.subresourceRange.aspectMask = VulkanUtilities::GetAspectFlags(m_TextureDescriptions[textureIndex].Format);
        l_Barrier.subresourceRange.baseMipLevel = 0;
        l_Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        l_Barrier.subresourceRange.baseArrayLayer = 0;
        l_Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        const bool l_IsAliased = textureIndex < m_AliasedTextures.size() && m_AliasedTextures[textureIndex];

        if (current.Layout == VK_IMAGE_LAYOUT_UNDEFINED && l_IsAliased)
        {
            l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            l_Barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        }

        out.push_back(l_Barrier);
    }

    void VulkanRenderGraph::AppendBufferBarrierIfNeeded(std::vector<VkBufferMemoryBarrier2>& out, uint32_t bufferIndex, const BufferState& current, const BufferState& next)
    {
        if (bufferIndex >= m_Buffers.size())
        {
            return;
        }

        auto l_Buffer = std::dynamic_pointer_cast<VulkanBuffer>(m_Buffers[bufferIndex]);
        if (!l_Buffer)
        {
            return;
        }

        if (current.Stage == next.Stage && current.Access == next.Access)
        {
            return;
        }

        VkBufferMemoryBarrier2 l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;

        l_Barrier.srcStageMask = current.Stage;
        l_Barrier.srcAccessMask = current.Access;
        l_Barrier.dstStageMask = next.Stage;
        l_Barrier.dstAccessMask = next.Access;

        l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        l_Barrier.buffer = l_Buffer->GetBuffer();
        l_Barrier.offset = 0;
        l_Barrier.size = VK_WHOLE_SIZE;

        out.push_back(l_Barrier);
    }

    void VulkanRenderGraph::ProcessTextureUsage(uint32_t passIndex, uint32_t textureIndex, RenderGraphAccess access, std::vector<ImageState>& currentStates)
    {
        if (passIndex >= m_PassBarriers.size())
        {
            return;
        }

        if (textureIndex >= currentStates.size() || textureIndex >= m_TextureDescriptions.size())
        {
            return;
        }

        const ImageState l_NextState = GetImageState(access, m_TextureDescriptions[textureIndex].Format);

        AppendTextureBarrierIfNeeded(m_PassBarriers[passIndex].ImageBarriers, textureIndex, currentStates[textureIndex], l_NextState);

        currentStates[textureIndex] = l_NextState;
    }

    void VulkanRenderGraph::ProcessBufferUsage(
        uint32_t passIndex,
        uint32_t bufferIndex,
        RenderGraphAccess access,
        std::vector<BufferState>& currentStates)
    {
        if (passIndex >= m_PassBarriers.size())
        {
            return;
        }

        if (bufferIndex >= currentStates.size() || bufferIndex >= m_BufferDescriptions.size())
        {
            return;
        }

        const BufferState l_NextState = GetBufferState(access);

        AppendBufferBarrierIfNeeded(m_PassBarriers[passIndex].BufferBarriers, bufferIndex, currentStates[bufferIndex], l_NextState);

        currentStates[bufferIndex] = l_NextState;
    }
}