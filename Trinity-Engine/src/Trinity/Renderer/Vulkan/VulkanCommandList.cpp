#include "Trinity/Renderer/Vulkan/VulkanCommandList.h"

#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanDescriptorSet.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Utilities/Log.h"

#include <cstring>
#include <vector>

namespace Trinity
{
    namespace
    {
        PFN_vkCmdBeginDebugUtilsLabelEXT s_BeginDebugLabel = nullptr;
        PFN_vkCmdEndDebugUtilsLabelEXT s_EndDebugLabel = nullptr;
        PFN_vkCmdInsertDebugUtilsLabelEXT s_InsertDebugLabel = nullptr;
        bool s_DebugUtilsLoaded = false;

        void LoadDebugUtils(VkDevice device)
        {
            if (s_DebugUtilsLoaded || device == VK_NULL_HANDLE)
            {
                return;
            }

            s_BeginDebugLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT"));
            s_EndDebugLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT"));
            s_InsertDebugLabel = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetDeviceProcAddr(device, "vkCmdInsertDebugUtilsLabelEXT"));
            s_DebugUtilsLoaded = true;
        }

        struct ResourceStateInfo
        {
            VkImageLayout Layout;
            VkPipelineStageFlags2 Stage;
            VkAccessFlags2 Access;
        };

        ResourceStateInfo TranslateImageState(ResourceState state)
        {
            switch (state)
            {
                case ResourceState::Undefined:
                    return { VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0 };
                case ResourceState::ColorAttachment:
                    return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT };
                case ResourceState::DepthStencilAttachment:
                    return { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT };
                case ResourceState::DepthStencilRead:
                    return { VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT };
                case ResourceState::ShaderRead:
                    return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT };
                case ResourceState::ShaderWrite:
                    return { VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT };
                case ResourceState::TransferSrc:
                    return { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT };
                case ResourceState::TransferDst:
                    return { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
                case ResourceState::Present:
                    return { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0 };
                default:
                    return { VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0 };
            }
        }

        struct BufferStateInfo
        {
            VkPipelineStageFlags2 Stage;
            VkAccessFlags2 Access;
        };

        BufferStateInfo TranslateBufferState(ResourceState state)
        {
            switch (state)
            {
                case ResourceState::ShaderRead:
                    return { VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT };
                case ResourceState::ShaderWrite:
                    return { VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT };
                case ResourceState::TransferSrc:
                    return { VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT };
                case ResourceState::TransferDst:
                    return { VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
                default:
                    return { VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0 };
            }
        }

        VkImageAspectFlags AspectFromFormat(VkFormat format)
        {
            switch (format)
            {
                case VK_FORMAT_D32_SFLOAT:
                case VK_FORMAT_D16_UNORM:
                    return VK_IMAGE_ASPECT_DEPTH_BIT;
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                case VK_FORMAT_D16_UNORM_S8_UINT:
                    return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                default:
                    return VK_IMAGE_ASPECT_COLOR_BIT;
            }
        }

        VkShaderStageFlags PushConstantStagesFor(const VulkanPipeline* pipeline, uint32_t offset, uint32_t size)
        {
            if (pipeline == nullptr)
            {
                return VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;
            }

            const auto& l_Spec = pipeline->GetSpecification();
            VkShaderStageFlags l_Stages = 0;

            for (const auto& it_Range : l_Spec.PushConstants)
            {
                if (offset >= it_Range.Offset && (offset + size) <= (it_Range.Offset + it_Range.Size))
                {
                    l_Stages |= VulkanUtilities::ToVkShaderStage(it_Range.Stage);
                }
            }

            return l_Stages != 0 ? l_Stages : (VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT);
        }
    }

    VulkanCommandList::~VulkanCommandList()
    {
        Shutdown();
    }

    VulkanCommandList::VulkanCommandList(VulkanCommandList&& other) noexcept
        : m_Device(other.m_Device), m_CommandBuffer(other.m_CommandBuffer), m_BoundGraphicsPipeline(other.m_BoundGraphicsPipeline), m_BoundComputePipeline(other.m_BoundComputePipeline), m_BoundLayout(other.m_BoundLayout)
    {
        other.m_Device = VK_NULL_HANDLE;
        other.m_CommandBuffer = VK_NULL_HANDLE;
        other.m_BoundGraphicsPipeline = nullptr;
        other.m_BoundComputePipeline = nullptr;
        other.m_BoundLayout = VK_NULL_HANDLE;
    }

    VulkanCommandList& VulkanCommandList::operator=(VulkanCommandList&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Shutdown();

        m_Device = other.m_Device;
        m_CommandBuffer = other.m_CommandBuffer;
        m_BoundGraphicsPipeline = other.m_BoundGraphicsPipeline;
        m_BoundComputePipeline = other.m_BoundComputePipeline;
        m_BoundLayout = other.m_BoundLayout;

        other.m_Device = VK_NULL_HANDLE;
        other.m_CommandBuffer = VK_NULL_HANDLE;
        other.m_BoundGraphicsPipeline = nullptr;
        other.m_BoundComputePipeline = nullptr;
        other.m_BoundLayout = VK_NULL_HANDLE;

        return *this;
    }

    void VulkanCommandList::Initialize(VkDevice device)
    {
        TR_CORE_TRACE("Initializing Vulkan Command List");

        m_Device = device;
        LoadDebugUtils(device);

        TR_CORE_TRACE("Vulkan Command List Initialized");
    }

    void VulkanCommandList::Shutdown()
    {
        m_CommandBuffer = VK_NULL_HANDLE;
        m_BoundGraphicsPipeline = nullptr;
        m_BoundComputePipeline = nullptr;
        m_BoundLayout = VK_NULL_HANDLE;
        m_Device = VK_NULL_HANDLE;
    }

    void VulkanCommandList::Reset(VkCommandBuffer commandBuffer)
    {
        m_CommandBuffer = commandBuffer;
        m_BoundGraphicsPipeline = nullptr;
        m_BoundComputePipeline = nullptr;
        m_BoundLayout = VK_NULL_HANDLE;
    }

    void VulkanCommandList::Invalidate()
    {
        m_CommandBuffer = VK_NULL_HANDLE;
        m_BoundGraphicsPipeline = nullptr;
        m_BoundComputePipeline = nullptr;
        m_BoundLayout = VK_NULL_HANDLE;
    }

    void VulkanCommandList::Begin()
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VulkanUtilities::VKCheck(vkBeginCommandBuffer(m_CommandBuffer, &l_BeginInfo), "Failed vkBeginCommandBuffer");
    }

    void VulkanCommandList::End()
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VulkanUtilities::VKCheck(vkEndCommandBuffer(m_CommandBuffer), "Failed vkEndCommandBuffer");
    }

    void VulkanCommandList::BeginRendering(const RenderingInfo& info)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        std::vector<VkRenderingAttachmentInfo> l_ColorAttachments(info.ColorAttachments.size());

        for (size_t i = 0; i < info.ColorAttachments.size(); i++)
        {
            const RenderingAttachment& a_Source = info.ColorAttachments[i];
            VkRenderingAttachmentInfo& a_Target = l_ColorAttachments[i];

            a_Target = {};
            a_Target.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            a_Target.imageView = a_Source.ColorTexture ? reinterpret_cast<VkImageView>(a_Source.ColorTexture->GetOpaqueHandle()) : VK_NULL_HANDLE;
            a_Target.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            a_Target.loadOp = a_Source.ClearOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            a_Target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            a_Target.clearValue.color.float32[0] = a_Source.ClearColor[0];
            a_Target.clearValue.color.float32[1] = a_Source.ClearColor[1];
            a_Target.clearValue.color.float32[2] = a_Source.ClearColor[2];
            a_Target.clearValue.color.float32[3] = a_Source.ClearColor[3];
        }

        VkRenderingAttachmentInfo l_DepthAttachment{};
        bool l_HasDepth = info.Depth.DepthTexture != nullptr;
        if (l_HasDepth)
        {
            l_DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            l_DepthAttachment.imageView = reinterpret_cast<VkImageView>(info.Depth.DepthTexture->GetOpaqueHandle());
            l_DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            l_DepthAttachment.loadOp = info.Depth.ClearOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            l_DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            l_DepthAttachment.clearValue.depthStencil.depth = info.Depth.ClearDepth;
            l_DepthAttachment.clearValue.depthStencil.stencil = info.Depth.ClearStencil;
        }

        VkRenderingInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        l_RenderingInfo.renderArea.offset = { 0, 0 };
        l_RenderingInfo.renderArea.extent = { info.Width, info.Height };
        l_RenderingInfo.layerCount = 1;
        l_RenderingInfo.colorAttachmentCount = static_cast<uint32_t>(l_ColorAttachments.size());
        l_RenderingInfo.pColorAttachments = l_ColorAttachments.empty() ? nullptr : l_ColorAttachments.data();
        l_RenderingInfo.pDepthAttachment = l_HasDepth ? &l_DepthAttachment : nullptr;

        vkCmdBeginRendering(m_CommandBuffer, &l_RenderingInfo);
    }

    void VulkanCommandList::EndRendering()
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        vkCmdEndRendering(m_CommandBuffer);
    }

    void VulkanCommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VkViewport l_Viewport{};
        l_Viewport.x = x;
        l_Viewport.y = y;
        l_Viewport.width = width;
        l_Viewport.height = height;
        l_Viewport.minDepth = minDepth;
        l_Viewport.maxDepth = maxDepth;

        vkCmdSetViewport(m_CommandBuffer, 0, 1, &l_Viewport);
    }

    void VulkanCommandList::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VkRect2D l_Scissor{};
        l_Scissor.offset = { x, y };
        l_Scissor.extent = { width, height };

        vkCmdSetScissor(m_CommandBuffer, 0, 1, &l_Scissor);
    }

    void VulkanCommandList::BindPipeline(const std::shared_ptr<Pipeline>& pipeline)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || pipeline == nullptr)
        {
            return;
        }

        auto* a_VulkanPipeline = dynamic_cast<VulkanPipeline*>(pipeline.get());
        if (a_VulkanPipeline == nullptr)
        {
            return;
        }

        m_BoundGraphicsPipeline = a_VulkanPipeline;
        m_BoundLayout = a_VulkanPipeline->GetLayout();
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, a_VulkanPipeline->GetPipeline());
    }

    void VulkanCommandList::BindComputePipeline(const std::shared_ptr<ComputePipeline>& pipeline)
    {
        TR_CORE_ERROR("VulkanCommandList::BindComputePipeline is not yet implemented (Phase 5.6 pending)");
        (void)pipeline;
    }

    void VulkanCommandList::BindDescriptorSet(uint32_t setIndex, const std::shared_ptr<DescriptorSet>& set, uint32_t dynamicOffsetCount, const uint32_t* dynamicOffsets)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || set == nullptr || m_BoundLayout == VK_NULL_HANDLE)
        {
            return;
        }

        auto* a_VulkanSet = dynamic_cast<VulkanDescriptorSet*>(set.get());
        if (a_VulkanSet == nullptr)
        {
            return;
        }

        a_VulkanSet->Flush();

        VkDescriptorSet l_Sets[] = { a_VulkanSet->GetVkSet() };
        const VkPipelineBindPoint l_BindPoint = (m_BoundComputePipeline != nullptr) ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

        vkCmdBindDescriptorSets(m_CommandBuffer, l_BindPoint, m_BoundLayout, setIndex, 1, l_Sets, dynamicOffsetCount, dynamicOffsets);
    }

    void VulkanCommandList::PushConstants(uint32_t offset, uint32_t size, const void* data)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || m_BoundLayout == VK_NULL_HANDLE)
        {
            return;
        }

        VkShaderStageFlags l_Stages = PushConstantStagesFor(m_BoundGraphicsPipeline, offset, size);
        vkCmdPushConstants(m_CommandBuffer, m_BoundLayout, l_Stages, offset, size, data);
    }

    void VulkanCommandList::BindVertexBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer, uint64_t offset)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || buffer == nullptr)
        {
            return;
        }

        auto* a_VulkanBuffer = dynamic_cast<VulkanBuffer*>(buffer.get());
        if (a_VulkanBuffer == nullptr)
        {
            return;
        }

        VkBuffer l_Buffers[] = { a_VulkanBuffer->GetBuffer() };
        VkDeviceSize l_Offsets[] = { offset };
        vkCmdBindVertexBuffers(m_CommandBuffer, binding, 1, l_Buffers, l_Offsets);
    }

    void VulkanCommandList::BindIndexBuffer(const std::shared_ptr<Buffer>& buffer, uint64_t offset, bool indexType32Bit)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || buffer == nullptr)
        {
            return;
        }

        auto* a_VulkanBuffer = dynamic_cast<VulkanBuffer*>(buffer.get());
        if (a_VulkanBuffer == nullptr)
        {
            return;
        }

        vkCmdBindIndexBuffer(m_CommandBuffer, a_VulkanBuffer->GetBuffer(), offset, indexType32Bit ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
    }

    void VulkanCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommandList::DrawIndexedIndirect(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || argBuffer == nullptr)
        {
            return;
        }

        auto* a_Args = dynamic_cast<VulkanBuffer*>(argBuffer.get());
        if (a_Args == nullptr)
        {
            return;
        }

        vkCmdDrawIndexedIndirect(m_CommandBuffer, a_Args->GetBuffer(), offset, drawCount, stride);
    }

    void VulkanCommandList::DrawIndexedIndirectCount(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset, const std::shared_ptr<Buffer>& countBuffer, uint64_t countOffset, uint32_t maxDraws, uint32_t stride)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || argBuffer == nullptr || countBuffer == nullptr)
        {
            return;
        }

        auto* a_Args = dynamic_cast<VulkanBuffer*>(argBuffer.get());
        auto* a_Count = dynamic_cast<VulkanBuffer*>(countBuffer.get());
        if (a_Args == nullptr || a_Count == nullptr)
        {
            return;
        }

        vkCmdDrawIndexedIndirectCount(m_CommandBuffer, a_Args->GetBuffer(), offset, a_Count->GetBuffer(), countOffset, maxDraws, stride);
    }

    void VulkanCommandList::Dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        vkCmdDispatch(m_CommandBuffer, groupX, groupY, groupZ);
    }

    void VulkanCommandList::DispatchIndirect(const std::shared_ptr<Buffer>& argBuffer, uint64_t offset)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || argBuffer == nullptr)
        {
            return;
        }

        auto* a_Args = dynamic_cast<VulkanBuffer*>(argBuffer.get());
        if (a_Args == nullptr)
        {
            return;
        }

        vkCmdDispatchIndirect(m_CommandBuffer, a_Args->GetBuffer(), offset);
    }

    void VulkanCommandList::TransitionResource(const std::shared_ptr<Texture>& texture, ResourceState before, ResourceState after)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || texture == nullptr)
        {
            return;
        }

        auto* a_VulkanTexture = dynamic_cast<VulkanTexture*>(texture.get());
        if (a_VulkanTexture == nullptr)
        {
            return;
        }

        ResourceStateInfo l_Before = TranslateImageState(before);
        ResourceStateInfo l_After = TranslateImageState(after);

        VkImageMemoryBarrier2 l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        l_Barrier.srcStageMask = l_Before.Stage;
        l_Barrier.srcAccessMask = l_Before.Access;
        l_Barrier.dstStageMask = l_After.Stage;
        l_Barrier.dstAccessMask = l_After.Access;
        l_Barrier.oldLayout = l_Before.Layout;
        l_Barrier.newLayout = l_After.Layout;
        l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.image = a_VulkanTexture->GetImage();
        l_Barrier.subresourceRange.aspectMask = AspectFromFormat(a_VulkanTexture->GetVkFormat());
        l_Barrier.subresourceRange.baseMipLevel = 0;
        l_Barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        l_Barrier.subresourceRange.baseArrayLayer = 0;
        l_Barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VkDependencyInfo l_DependencyInfo{};
        l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_DependencyInfo.imageMemoryBarrierCount = 1;
        l_DependencyInfo.pImageMemoryBarriers = &l_Barrier;

        vkCmdPipelineBarrier2(m_CommandBuffer, &l_DependencyInfo);
    }

    void VulkanCommandList::BufferBarrier(const std::shared_ptr<Buffer>& buffer, ResourceState before, ResourceState after)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || buffer == nullptr)
        {
            return;
        }

        auto* a_VulkanBuffer = dynamic_cast<VulkanBuffer*>(buffer.get());
        if (a_VulkanBuffer == nullptr)
        {
            return;
        }

        BufferStateInfo l_Before = TranslateBufferState(before);
        BufferStateInfo l_After = TranslateBufferState(after);

        VkBufferMemoryBarrier2 l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        l_Barrier.srcStageMask = l_Before.Stage;
        l_Barrier.srcAccessMask = l_Before.Access;
        l_Barrier.dstStageMask = l_After.Stage;
        l_Barrier.dstAccessMask = l_After.Access;
        l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.buffer = a_VulkanBuffer->GetBuffer();
        l_Barrier.offset = 0;
        l_Barrier.size = VK_WHOLE_SIZE;

        VkDependencyInfo l_DependencyInfo{};
        l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_DependencyInfo.bufferMemoryBarrierCount = 1;
        l_DependencyInfo.pBufferMemoryBarriers = &l_Barrier;

        vkCmdPipelineBarrier2(m_CommandBuffer, &l_DependencyInfo);
    }

    void VulkanCommandList::CopyBuffer(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Buffer>& dst, uint64_t srcOffset, uint64_t dstOffset, uint64_t size)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || src == nullptr || dst == nullptr)
        {
            return;
        }

        auto* a_Src = dynamic_cast<VulkanBuffer*>(src.get());
        auto* a_Dst = dynamic_cast<VulkanBuffer*>(dst.get());
        if (a_Src == nullptr || a_Dst == nullptr)
        {
            return;
        }

        VkBufferCopy l_Region{};
        l_Region.srcOffset = srcOffset;
        l_Region.dstOffset = dstOffset;
        l_Region.size = size;

        vkCmdCopyBuffer(m_CommandBuffer, a_Src->GetBuffer(), a_Dst->GetBuffer(), 1, &l_Region);
    }

    void VulkanCommandList::CopyBufferToTexture(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Texture>& dst, uint32_t mipLevel, uint32_t arrayLayer)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || src == nullptr || dst == nullptr)
        {
            return;
        }

        auto* a_Src = dynamic_cast<VulkanBuffer*>(src.get());
        auto* a_Dst = dynamic_cast<VulkanTexture*>(dst.get());
        if (a_Src == nullptr || a_Dst == nullptr)
        {
            return;
        }

        VkBufferImageCopy l_Region{};
        l_Region.bufferOffset = 0;
        l_Region.bufferRowLength = 0;
        l_Region.bufferImageHeight = 0;
        l_Region.imageSubresource.aspectMask = AspectFromFormat(a_Dst->GetVkFormat());
        l_Region.imageSubresource.mipLevel = mipLevel;
        l_Region.imageSubresource.baseArrayLayer = arrayLayer;
        l_Region.imageSubresource.layerCount = 1;
        l_Region.imageOffset = { 0, 0, 0 };
        l_Region.imageExtent = { a_Dst->GetWidth() >> mipLevel, a_Dst->GetHeight() >> mipLevel, 1 };

        vkCmdCopyBufferToImage(m_CommandBuffer, a_Src->GetBuffer(), a_Dst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_Region);
    }

    void VulkanCommandList::BlitTexture(const std::shared_ptr<Texture>& src, const std::shared_ptr<Texture>& dst)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || src == nullptr || dst == nullptr)
        {
            return;
        }

        auto* a_Src = dynamic_cast<VulkanTexture*>(src.get());
        auto* a_Dst = dynamic_cast<VulkanTexture*>(dst.get());
        if (a_Src == nullptr || a_Dst == nullptr)
        {
            return;
        }

        VkImageBlit l_Blit{};
        l_Blit.srcSubresource.aspectMask = AspectFromFormat(a_Src->GetVkFormat());
        l_Blit.srcSubresource.mipLevel = 0;
        l_Blit.srcSubresource.baseArrayLayer = 0;
        l_Blit.srcSubresource.layerCount = 1;
        l_Blit.srcOffsets[0] = { 0, 0, 0 };
        l_Blit.srcOffsets[1] = { static_cast<int32_t>(a_Src->GetWidth()), static_cast<int32_t>(a_Src->GetHeight()), 1 };
        l_Blit.dstSubresource.aspectMask = AspectFromFormat(a_Dst->GetVkFormat());
        l_Blit.dstSubresource.mipLevel = 0;
        l_Blit.dstSubresource.baseArrayLayer = 0;
        l_Blit.dstSubresource.layerCount = 1;
        l_Blit.dstOffsets[0] = { 0, 0, 0 };
        l_Blit.dstOffsets[1] = { static_cast<int32_t>(a_Dst->GetWidth()), static_cast<int32_t>(a_Dst->GetHeight()), 1 };

        vkCmdBlitImage(m_CommandBuffer, a_Src->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, a_Dst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_Blit, VK_FILTER_LINEAR);
    }

    void VulkanCommandList::GenerateMips(const std::shared_ptr<Texture>& texture)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || texture == nullptr)
        {
            return;
        }

        auto* a_VulkanTexture = dynamic_cast<VulkanTexture*>(texture.get());
        if (a_VulkanTexture == nullptr)
        {
            return;
        }

        if (VulkanUtilities::IsBlockCompressedFormat(a_VulkanTexture->GetSpecification().Format))
        {
            TR_CORE_WARN("GenerateMips called on block-compressed texture [{}] - skipping (BC content must include mips)", a_VulkanTexture->GetSpecification().DebugName);
            return;
        }

        const uint32_t l_MipLevels = a_VulkanTexture->GetMipLevels();
        if (l_MipLevels <= 1)
        {
            return;
        }

        const uint32_t l_LayerCount = a_VulkanTexture->GetArrayLayers();
        const VkImageAspectFlags l_Aspect = AspectFromFormat(a_VulkanTexture->GetVkFormat());
        VkImage l_Image = a_VulkanTexture->GetImage();

        int32_t l_MipWidth = static_cast<int32_t>(a_VulkanTexture->GetWidth());
        int32_t l_MipHeight = static_cast<int32_t>(a_VulkanTexture->GetHeight());

        for (uint32_t i = 1; i < l_MipLevels; i++)
        {
            VkImageMemoryBarrier2 l_SrcBarrier{};
            l_SrcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            l_SrcBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            l_SrcBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            l_SrcBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            l_SrcBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            l_SrcBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            l_SrcBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            l_SrcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            l_SrcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            l_SrcBarrier.image = l_Image;
            l_SrcBarrier.subresourceRange.aspectMask = l_Aspect;
            l_SrcBarrier.subresourceRange.baseMipLevel = i - 1;
            l_SrcBarrier.subresourceRange.levelCount = 1;
            l_SrcBarrier.subresourceRange.baseArrayLayer = 0;
            l_SrcBarrier.subresourceRange.layerCount = l_LayerCount;

            VkDependencyInfo l_SrcDependency{};
            l_SrcDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            l_SrcDependency.imageMemoryBarrierCount = 1;
            l_SrcDependency.pImageMemoryBarriers = &l_SrcBarrier;
            vkCmdPipelineBarrier2(m_CommandBuffer, &l_SrcDependency);

            VkImageBlit l_Blit{};
            l_Blit.srcSubresource.aspectMask = l_Aspect;
            l_Blit.srcSubresource.mipLevel = i - 1;
            l_Blit.srcSubresource.baseArrayLayer = 0;
            l_Blit.srcSubresource.layerCount = l_LayerCount;
            l_Blit.srcOffsets[0] = { 0, 0, 0 };
            l_Blit.srcOffsets[1] = { l_MipWidth, l_MipHeight, 1 };
            l_Blit.dstSubresource.aspectMask = l_Aspect;
            l_Blit.dstSubresource.mipLevel = i;
            l_Blit.dstSubresource.baseArrayLayer = 0;
            l_Blit.dstSubresource.layerCount = l_LayerCount;
            l_Blit.dstOffsets[0] = { 0, 0, 0 };
            l_Blit.dstOffsets[1] = { l_MipWidth > 1 ? l_MipWidth / 2 : 1, l_MipHeight > 1 ? l_MipHeight / 2 : 1, 1 };

            vkCmdBlitImage(m_CommandBuffer, l_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, l_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_Blit, VK_FILTER_LINEAR);

            VkImageMemoryBarrier2 l_FinalSrcBarrier{};
            l_FinalSrcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            l_FinalSrcBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            l_FinalSrcBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            l_FinalSrcBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            l_FinalSrcBarrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
            l_FinalSrcBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            l_FinalSrcBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            l_FinalSrcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            l_FinalSrcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            l_FinalSrcBarrier.image = l_Image;
            l_FinalSrcBarrier.subresourceRange.aspectMask = l_Aspect;
            l_FinalSrcBarrier.subresourceRange.baseMipLevel = i - 1;
            l_FinalSrcBarrier.subresourceRange.levelCount = 1;
            l_FinalSrcBarrier.subresourceRange.baseArrayLayer = 0;
            l_FinalSrcBarrier.subresourceRange.layerCount = l_LayerCount;

            VkDependencyInfo l_FinalSrcDependency{};
            l_FinalSrcDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            l_FinalSrcDependency.imageMemoryBarrierCount = 1;
            l_FinalSrcDependency.pImageMemoryBarriers = &l_FinalSrcBarrier;
            vkCmdPipelineBarrier2(m_CommandBuffer, &l_FinalSrcDependency);

            if (l_MipWidth > 1)
            {
                l_MipWidth /= 2;
            }

            if (l_MipHeight > 1)
            {
                l_MipHeight /= 2;
            }
        }

        VkImageMemoryBarrier2 l_FinalDstBarrier{};
        l_FinalDstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        l_FinalDstBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        l_FinalDstBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        l_FinalDstBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        l_FinalDstBarrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        l_FinalDstBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        l_FinalDstBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        l_FinalDstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_FinalDstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_FinalDstBarrier.image = l_Image;
        l_FinalDstBarrier.subresourceRange.aspectMask = l_Aspect;
        l_FinalDstBarrier.subresourceRange.baseMipLevel = l_MipLevels - 1;
        l_FinalDstBarrier.subresourceRange.levelCount = 1;
        l_FinalDstBarrier.subresourceRange.baseArrayLayer = 0;
        l_FinalDstBarrier.subresourceRange.layerCount = l_LayerCount;

        VkDependencyInfo l_FinalDstDependency{};
        l_FinalDstDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_FinalDstDependency.imageMemoryBarrierCount = 1;
        l_FinalDstDependency.pImageMemoryBarriers = &l_FinalDstBarrier;
        vkCmdPipelineBarrier2(m_CommandBuffer, &l_FinalDstDependency);
    }

    void VulkanCommandList::WriteTimestamp(const std::shared_ptr<QueryPool>& pool, uint32_t index)
    {
        TR_CORE_ERROR("VulkanCommandList::WriteTimestamp is not yet implemented (Phase 5.10 pending)");
        (void)pool;
        (void)index;
    }

    void VulkanCommandList::ResetQueryPool(const std::shared_ptr<QueryPool>& pool, uint32_t firstIndex, uint32_t count)
    {
        TR_CORE_ERROR("VulkanCommandList::ResetQueryPool is not yet implemented (Phase 5.10 pending)");
        (void)pool;
        (void)firstIndex;
        (void)count;
    }

    void VulkanCommandList::BeginDebugLabel(const std::string& name, const float color[4])
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || s_BeginDebugLabel == nullptr)
        {
            return;
        }

        VkDebugUtilsLabelEXT l_Label{};
        l_Label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        l_Label.pLabelName = name.c_str();
        if (color != nullptr)
        {
            l_Label.color[0] = color[0];
            l_Label.color[1] = color[1];
            l_Label.color[2] = color[2];
            l_Label.color[3] = color[3];
        }

        s_BeginDebugLabel(m_CommandBuffer, &l_Label);
    }

    void VulkanCommandList::EndDebugLabel()
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || s_EndDebugLabel == nullptr)
        {
            return;
        }

        s_EndDebugLabel(m_CommandBuffer);
    }

    void VulkanCommandList::InsertDebugMarker(const std::string& name)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || s_InsertDebugLabel == nullptr)
        {
            return;
        }

        VkDebugUtilsLabelEXT l_Label{};
        l_Label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        l_Label.pLabelName = name.c_str();

        s_InsertDebugLabel(m_CommandBuffer, &l_Label);
    }
}