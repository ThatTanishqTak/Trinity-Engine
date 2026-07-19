#include <Trinity/Renderer/Backends/Vulkan/VulkanCommandList.h>

#include <vector>

#include <Trinity/Renderer/Backends/Vulkan/VulkanDevice.h>
#include <Trinity/Renderer/Backends/Vulkan/VulkanUtilities.h>
#include <Trinity/Core/Log.h>

namespace Trinity
{
    struct StateInfo
    {
        VkImageLayout Layout;
        VkPipelineStageFlags2 Stage;
        VkAccessFlags2 Access;
    };

    static StateInfo ResolveState(ResourceState state)
    {
        switch (state)
        {
            case ResourceState::RenderTarget:
                return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT };
            case ResourceState::DepthStencil:
                return { VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT };
            case ResourceState::ShaderResource:
                return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT };
            case ResourceState::CopySource:
                return { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT };
            case ResourceState::CopyDestination:
                return { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
            case ResourceState::Present:
                return { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0 };
            case ResourceState::General:
                return { VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT };
            case ResourceState::Undefined:
            default:
                return { VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0 };
        }
    }

    VulkanCommandList::VulkanCommandList(VulkanDevice& device) : m_Device(device)
    {
        VkCommandBufferAllocateInfo l_CommandBufferAllocateInfo{};
        l_CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_CommandBufferAllocateInfo.commandPool = m_Device.GetCommands().GetPool();
        l_CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_CommandBufferAllocateInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_Device.GetHandle(), &l_CommandBufferAllocateInfo, &m_CommandBuffer) != VK_SUCCESS)
        {
            ("VulkanCommandList: vkAllocateCommandBuffers failed");
        }

        VkDescriptorPoolSize l_PoolSizes[2]{};
        l_PoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_PoolSizes[0].descriptorCount = 1024;
        l_PoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_PoolSizes[1].descriptorCount = 1024;

        VkDescriptorPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_PoolInfo.maxSets = 1024;
        l_PoolInfo.poolSizeCount = 2;
        l_PoolInfo.pPoolSizes = l_PoolSizes;

        if (vkCreateDescriptorPool(m_Device.GetHandle(), &l_PoolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
        {
            ("VulkanCommandList: vkCreateDescriptorPool failed");
        }
    }

    VulkanCommandList::~VulkanCommandList()
    {
        if (m_DescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_Device.GetHandle(), m_DescriptorPool, nullptr);
        }

        if (m_CommandBuffer != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(m_Device.GetHandle(), m_Device.GetCommands().GetPool(), 1, &m_CommandBuffer);
        }
    }

    void VulkanCommandList::Begin()
    {
        vkResetCommandBuffer(m_CommandBuffer, 0);

        VkCommandBufferBeginInfo l_CommandBufferBeginInfo{};
        l_CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(m_CommandBuffer, &l_CommandBufferBeginInfo);

        if (m_DescriptorPool != VK_NULL_HANDLE)
        {
            vkResetDescriptorPool(m_Device.GetHandle(), m_DescriptorPool, 0);
        }

        m_CurrentLayout = VK_NULL_HANDLE;
        m_CurrentSetLayouts.clear();
    }

    void VulkanCommandList::End()
    {
        vkEndCommandBuffer(m_CommandBuffer);
    }

    void VulkanCommandList::BeginRendering(const RenderingInfo& renderingInfo)
    {
        std::vector<VkRenderingAttachmentInfo> l_ColorAttachments;
        l_ColorAttachments.reserve(renderingInfo.ColorAttachmentCount);

        for (uint32_t l_Index = 0; l_Index < renderingInfo.ColorAttachmentCount; ++l_Index)
        {
            const RenderingAttachment& l_Attachment = renderingInfo.ColorAttachments[l_Index];
            VulkanTextureResource* l_Texture = m_Device.GetTexture(l_Attachment.Target);
            if (l_Texture == nullptr)
            {
                continue;
            }

            VkImageView l_AttachmentView = l_Texture->View;
            if (l_Attachment.MipLevel != 0 || l_Attachment.ArrayLayer != 0)
            {
                l_AttachmentView = m_Device.GetRenderTargetView(l_Attachment.Target, l_Attachment.MipLevel, l_Attachment.ArrayLayer);
            }

            VkRenderingAttachmentInfo l_RenderingAttachmentInfo{};
            l_RenderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            l_RenderingAttachmentInfo.imageView = l_AttachmentView;
            l_RenderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            l_RenderingAttachmentInfo.loadOp = l_Attachment.Clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            l_RenderingAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            l_RenderingAttachmentInfo.clearValue.color = { { l_Attachment.ClearColor[0], l_Attachment.ClearColor[1], l_Attachment.ClearColor[2], l_Attachment.ClearColor[3] } };
            l_ColorAttachments.push_back(l_RenderingAttachmentInfo);
        }

        VkRenderingAttachmentInfo l_RenderingDepthAttachmentInfo{};
        bool l_HasDepth = false;
        if (renderingInfo.Depth != nullptr)
        {
            VulkanTextureResource* l_DepthTexture = m_Device.GetTexture(renderingInfo.Depth->Target);
            if (l_DepthTexture != nullptr)
            {
                l_RenderingDepthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                l_RenderingDepthAttachmentInfo.imageView = l_DepthTexture->View;
                l_RenderingDepthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                l_RenderingDepthAttachmentInfo.loadOp = renderingInfo.Depth->Clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                l_RenderingDepthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                l_RenderingDepthAttachmentInfo.clearValue.depthStencil.depth = renderingInfo.Depth->ClearDepth;
                l_HasDepth = true;
            }
        }

        VkRenderingInfo l_RenderingInfo{};
        l_RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        l_RenderingInfo.renderArea.offset = { 0, 0 };
        l_RenderingInfo.renderArea.extent = { renderingInfo.Width, renderingInfo.Height };
        l_RenderingInfo.layerCount = 1;
        l_RenderingInfo.colorAttachmentCount = static_cast<uint32_t>(l_ColorAttachments.size());
        l_RenderingInfo.pColorAttachments = l_ColorAttachments.empty() ? nullptr : l_ColorAttachments.data();
        l_RenderingInfo.pDepthAttachment = l_HasDepth ? &l_RenderingDepthAttachmentInfo : nullptr;

        vkCmdBeginRendering(m_CommandBuffer, &l_RenderingInfo);
    }

    void VulkanCommandList::EndRendering()
    {
        vkCmdEndRendering(m_CommandBuffer);
    }

    void VulkanCommandList::SetViewport(const Viewport& viewport)
    {
        VkViewport l_Viewport{};
        l_Viewport.x = viewport.X;
        l_Viewport.y = viewport.Y + viewport.Height;
        l_Viewport.width = viewport.Width;
        l_Viewport.height = -viewport.Height;
        l_Viewport.minDepth = viewport.MinDepth;
        l_Viewport.maxDepth = viewport.MaxDepth;

        vkCmdSetViewport(m_CommandBuffer, 0, 1, &l_Viewport);
    }

    void VulkanCommandList::SetScissor(const Scissor& scissor)
    {
        VkRect2D l_Scissor{};
        l_Scissor.offset = { scissor.X, scissor.Y };
        l_Scissor.extent = { scissor.Width, scissor.Height };

        vkCmdSetScissor(m_CommandBuffer, 0, 1, &l_Scissor);
    }

    void VulkanCommandList::BindPipeline(PipelineHandle pipeline)
    {
        VulkanPipelineResource* l_Pipeline = m_Device.GetPipeline(pipeline);
        if (l_Pipeline == nullptr)
        {
            return;
        }

        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, l_Pipeline->Pipeline);
        m_CurrentLayout = l_Pipeline->Layout;
        m_CurrentSetLayouts = l_Pipeline->SetLayouts;
    }

    void VulkanCommandList::BindVertexBuffer(BufferHandle buffer, uint64_t offset)
    {
        VulkanBufferResource* l_Buffer = m_Device.GetBuffer(buffer);
        if (l_Buffer == nullptr)
        {
            return;
        }

        VkBuffer l_Handle = l_Buffer->Buffer;
        VkDeviceSize l_Offset = offset;
        vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &l_Handle, &l_Offset);
    }

    void VulkanCommandList::BindIndexBuffer(BufferHandle buffer, uint64_t offset)
    {
        VulkanBufferResource* l_Buffer = m_Device.GetBuffer(buffer);
        if (l_Buffer == nullptr)
        {
            return;
        }

        vkCmdBindIndexBuffer(m_CommandBuffer, l_Buffer->Buffer, offset, VK_INDEX_TYPE_UINT32);
    }

    void VulkanCommandList::PushConstants(ShaderStage stages, uint32_t offset, uint32_t size, const void* data)
    {
        if (m_CurrentLayout == VK_NULL_HANDLE)
        {
            return;
        }

        vkCmdPushConstants(m_CommandBuffer, m_CurrentLayout, VulkanUtilities::ToVkShaderStages(stages), offset, size, data);
    }

    void VulkanCommandList::BindTexture(uint32_t set, uint32_t binding, TextureHandle texture, SamplerHandle sampler)
    {
        if (m_CurrentLayout == VK_NULL_HANDLE || set >= m_CurrentSetLayouts.size())
        {
            return;
        }

        VulkanTextureResource* l_Texture = m_Device.GetTexture(texture);
        VulkanSamplerResource* l_Sampler = m_Device.GetSampler(sampler);
        if (l_Texture == nullptr || l_Sampler == nullptr)
        {
            return;
        }

        VkDescriptorSetAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_AllocateInfo.descriptorPool = m_DescriptorPool;
        l_AllocateInfo.descriptorSetCount = 1;
        l_AllocateInfo.pSetLayouts = &m_CurrentSetLayouts[set];

        VkDescriptorSet l_DescriptorSet = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(m_Device.GetHandle(), &l_AllocateInfo, &l_DescriptorSet) != VK_SUCCESS)
        {
            ("VulkanCommandList: vkAllocateDescriptorSets failed");
            return;
        }

        VkDescriptorImageInfo l_ImageInfo{};
        l_ImageInfo.sampler = l_Sampler->Sampler;
        l_ImageInfo.imageView = l_Texture->View;
        l_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet l_Write{};
        l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_Write.dstSet = l_DescriptorSet;
        l_Write.dstBinding = binding;
        l_Write.dstArrayElement = 0;
        l_Write.descriptorCount = 1;
        l_Write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_Write.pImageInfo = &l_ImageInfo;

        vkUpdateDescriptorSets(m_Device.GetHandle(), 1, &l_Write, 0, nullptr);

        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CurrentLayout, set, 1, &l_DescriptorSet, 0, nullptr);
    }

    void VulkanCommandList::BindUniformBuffer(uint32_t set, uint32_t binding, BufferHandle buffer, uint64_t offset, uint64_t size)
    {
        if (m_CurrentLayout == VK_NULL_HANDLE || set >= m_CurrentSetLayouts.size())
        {
            return;
        }

        VulkanBufferResource* l_Buffer = m_Device.GetBuffer(buffer);
        if (l_Buffer == nullptr)
        {
            return;
        }

        VkDescriptorSetAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_AllocateInfo.descriptorPool = m_DescriptorPool;
        l_AllocateInfo.descriptorSetCount = 1;
        l_AllocateInfo.pSetLayouts = &m_CurrentSetLayouts[set];

        VkDescriptorSet l_DescriptorSet = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(m_Device.GetHandle(), &l_AllocateInfo, &l_DescriptorSet) != VK_SUCCESS)
        {
            ("VulkanCommandList: vkAllocateDescriptorSets failed");
            return;
        }

        VkDescriptorBufferInfo l_BufferInfo{};
        l_BufferInfo.buffer = l_Buffer->Buffer;
        l_BufferInfo.offset = offset;
        l_BufferInfo.range = size;

        VkWriteDescriptorSet l_Write{};
        l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_Write.dstSet = l_DescriptorSet;
        l_Write.dstBinding = binding;
        l_Write.dstArrayElement = 0;
        l_Write.descriptorCount = 1;
        l_Write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_Write.pBufferInfo = &l_BufferInfo;

        vkUpdateDescriptorSets(m_Device.GetHandle(), 1, &l_Write, 0, nullptr);

        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CurrentLayout, set, 1, &l_DescriptorSet, 0, nullptr);
    }

    void VulkanCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommandList::TransitionTexture(TextureHandle texture, ResourceState from, ResourceState to)
    {
        VulkanTextureResource* l_Texture = m_Device.GetTexture(texture);
        if (l_Texture == nullptr)
        {
            return;
        }

        if (from != ResourceState::Undefined && l_Texture->CurrentState != from)
        {
            ("VulkanCommandList: transition '{}' declared from {} but tracked state is {}", l_Texture->DebugName, static_cast<int>(from), static_cast<int>(l_Texture->CurrentState));
        }

        l_Texture->CurrentState = to;

        StateInfo l_From = ResolveState(from);
        StateInfo l_To = ResolveState(to);

        VkImageMemoryBarrier2 l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        l_Barrier.srcStageMask = l_From.Stage;
        l_Barrier.srcAccessMask = l_From.Access;
        l_Barrier.dstStageMask = l_To.Stage;
        l_Barrier.dstAccessMask = l_To.Access;
        l_Barrier.oldLayout = l_From.Layout;
        l_Barrier.newLayout = l_To.Layout;
        l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.image = l_Texture->Image;
        l_Barrier.subresourceRange.aspectMask = l_Texture->Aspect;
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
}