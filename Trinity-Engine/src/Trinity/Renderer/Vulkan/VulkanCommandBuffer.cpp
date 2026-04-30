#include "Trinity/Renderer/Vulkan/VulkanCommandBuffer.h"

#include "Trinity/Renderer/Vulkan/Resources/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanSampler.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanTexture.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

namespace Trinity
{
    namespace
    {
        VkShaderStageFlags ToVkShaderStageFlags(ShaderStageFlags stages)
        {
            VkShaderStageFlags l_Result = 0;
            if (stages & ShaderStageFlags::Vertex)
            {
                l_Result |= VK_SHADER_STAGE_VERTEX_BIT;
            }

            if (stages & ShaderStageFlags::Fragment)
            {
                l_Result |= VK_SHADER_STAGE_FRAGMENT_BIT;
            }

            if (stages & ShaderStageFlags::Compute)
            {
                l_Result |= VK_SHADER_STAGE_COMPUTE_BIT;
            }

            return l_Result;
        }

        VkIndexType ToVkIndexType(IndexType type)
        {
            switch (type)
            {
                case IndexType::UInt16:
                    return VK_INDEX_TYPE_UINT16;

                case IndexType::UInt32:
                    return VK_INDEX_TYPE_UINT32;
            }

            return VK_INDEX_TYPE_UINT32;
        }
    }

    VulkanCommandBuffer::~VulkanCommandBuffer()
    {
        Shutdown();
    }

    VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept : m_Device(other.m_Device), m_CommandBuffer(other.m_CommandBuffer), m_BoundPipeline(other.m_BoundPipeline), m_TransientDescriptorPool(other.m_TransientDescriptorPool)
    {
        other.m_Device = VK_NULL_HANDLE;
        other.m_CommandBuffer = VK_NULL_HANDLE;
        other.m_BoundPipeline = nullptr;
        other.m_TransientDescriptorPool = VK_NULL_HANDLE;
    }

    VulkanCommandBuffer& VulkanCommandBuffer::operator=(VulkanCommandBuffer&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Shutdown();

        m_Device = other.m_Device;
        m_CommandBuffer = other.m_CommandBuffer;
        m_BoundPipeline = other.m_BoundPipeline;
        m_TransientDescriptorPool = other.m_TransientDescriptorPool;

        other.m_Device = VK_NULL_HANDLE;
        other.m_CommandBuffer = VK_NULL_HANDLE;
        other.m_BoundPipeline = nullptr;
        other.m_TransientDescriptorPool = VK_NULL_HANDLE;

        return *this;
    }

    void VulkanCommandBuffer::Initialize(VkDevice device)
    {
        m_Device = device;

        constexpr uint32_t l_PerType = 256;
        VkDescriptorPoolSize l_PoolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, l_PerType },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, l_PerType },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, l_PerType },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, l_PerType },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, l_PerType }
        };

        constexpr uint32_t l_PoolSizeCount = static_cast<uint32_t>(sizeof(l_PoolSizes) / sizeof(l_PoolSizes[0]));

        VkDescriptorPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_PoolInfo.maxSets = l_PerType * l_PoolSizeCount;
        l_PoolInfo.poolSizeCount = l_PoolSizeCount;
        l_PoolInfo.pPoolSizes = l_PoolSizes;

        VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Device, &l_PoolInfo, nullptr, &m_TransientDescriptorPool), "Failed vkCreateDescriptorPool (transient)");
    }

    void VulkanCommandBuffer::Shutdown()
    {
        if (m_TransientDescriptorPool != VK_NULL_HANDLE && m_Device != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_Device, m_TransientDescriptorPool, nullptr);
        }

        m_TransientDescriptorPool = VK_NULL_HANDLE;
        m_CommandBuffer = VK_NULL_HANDLE;
        m_BoundPipeline = nullptr;
        m_Device = VK_NULL_HANDLE;
    }

    void VulkanCommandBuffer::Reset(VkCommandBuffer commandBuffer)
    {
        m_CommandBuffer = commandBuffer;
        m_BoundPipeline = nullptr;

        if (m_TransientDescriptorPool != VK_NULL_HANDLE)
        {
            vkResetDescriptorPool(m_Device, m_TransientDescriptorPool, 0);
        }
    }

    void VulkanCommandBuffer::Invalidate()
    {
        m_CommandBuffer = VK_NULL_HANDLE;
        m_BoundPipeline = nullptr;
    }

    void VulkanCommandBuffer::BindPipeline(Pipeline& pipeline)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        auto* a_VulkanPipeline = dynamic_cast<VulkanPipeline*>(&pipeline);
        if (a_VulkanPipeline == nullptr)
        {
            return;
        }

        m_BoundPipeline = a_VulkanPipeline;
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, a_VulkanPipeline->GetPipeline());
    }

    void VulkanCommandBuffer::BindVertexBuffer(uint32_t binding, Buffer& buffer, uint64_t offset)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        auto* a_VulkanBuffer = dynamic_cast<VulkanBuffer*>(&buffer);
        if (a_VulkanBuffer == nullptr)
        {
            return;
        }

        VkBuffer l_Buffers[] = { a_VulkanBuffer->GetBuffer() };
        VkDeviceSize l_Offsets[] = { offset };
        vkCmdBindVertexBuffers(m_CommandBuffer, binding, 1, l_Buffers, l_Offsets);
    }

    void VulkanCommandBuffer::BindIndexBuffer(Buffer& buffer, IndexType type, uint64_t offset)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        auto* a_VulkanBuffer = dynamic_cast<VulkanBuffer*>(&buffer);
        if (a_VulkanBuffer == nullptr)
        {
            return;
        }

        vkCmdBindIndexBuffer(m_CommandBuffer, a_VulkanBuffer->GetBuffer(), offset, ToVkIndexType(type));
    }

    void VulkanCommandBuffer::BindTexture(uint32_t set, uint32_t binding, Texture& texture, Sampler& sampler)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || m_BoundPipeline == nullptr || m_TransientDescriptorPool == VK_NULL_HANDLE)
        {
            return;
        }

        auto* a_VulkanTexture = dynamic_cast<VulkanTexture*>(&texture);
        auto* a_VulkanSampler = dynamic_cast<VulkanSampler*>(&sampler);
        if (a_VulkanTexture == nullptr || a_VulkanSampler == nullptr)
        {
            return;
        }

        VkDescriptorSetLayout l_SetLayout = m_BoundPipeline->GetDescriptorSetLayout();
        if (l_SetLayout == VK_NULL_HANDLE)
        {
            return;
        }

        VkDescriptorSetAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_AllocInfo.descriptorPool = m_TransientDescriptorPool;
        l_AllocInfo.descriptorSetCount = 1;
        l_AllocInfo.pSetLayouts = &l_SetLayout;

        VkDescriptorSet l_DescriptorSet = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(m_Device, &l_AllocInfo, &l_DescriptorSet) != VK_SUCCESS)
        {
            return;
        }

        VkDescriptorImageInfo l_ImageInfo{};
        l_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        l_ImageInfo.imageView = a_VulkanTexture->GetImageView();
        l_ImageInfo.sampler = a_VulkanSampler->GetSampler();

        VkWriteDescriptorSet l_Write{};
        l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_Write.dstSet = l_DescriptorSet;
        l_Write.dstBinding = binding;
        l_Write.dstArrayElement = 0;
        l_Write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        l_Write.descriptorCount = 1;
        l_Write.pImageInfo = &l_ImageInfo;

        vkUpdateDescriptorSets(m_Device, 1, &l_Write, 0, nullptr);

        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BoundPipeline->GetLayout(), set, 1, &l_DescriptorSet, 0, nullptr);
    }

    void VulkanCommandBuffer::BindUniformBuffer(uint32_t set, uint32_t binding, Buffer& buffer, uint64_t offset, uint64_t range)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || m_BoundPipeline == nullptr || m_TransientDescriptorPool == VK_NULL_HANDLE)
        {
            return;
        }

        auto* a_VulkanBuffer = dynamic_cast<VulkanBuffer*>(&buffer);
        if (a_VulkanBuffer == nullptr)
        {
            return;
        }

        VkDescriptorSetLayout l_SetLayout = m_BoundPipeline->GetDescriptorSetLayout();
        if (l_SetLayout == VK_NULL_HANDLE)
        {
            return;
        }

        VkDescriptorSetAllocateInfo l_AllocInfo{};
        l_AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_AllocInfo.descriptorPool = m_TransientDescriptorPool;
        l_AllocInfo.descriptorSetCount = 1;
        l_AllocInfo.pSetLayouts = &l_SetLayout;

        VkDescriptorSet l_DescriptorSet = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(m_Device, &l_AllocInfo, &l_DescriptorSet) != VK_SUCCESS)
        {
            return;
        }

        VkDescriptorBufferInfo l_BufferInfo{};
        l_BufferInfo.buffer = a_VulkanBuffer->GetBuffer();
        l_BufferInfo.offset = offset;
        l_BufferInfo.range = range;

        VkWriteDescriptorSet l_Write{};
        l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        l_Write.dstSet = l_DescriptorSet;
        l_Write.dstBinding = binding;
        l_Write.dstArrayElement = 0;
        l_Write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_Write.descriptorCount = 1;
        l_Write.pBufferInfo = &l_BufferInfo;

        vkUpdateDescriptorSets(m_Device, 1, &l_Write, 0, nullptr);

        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BoundPipeline->GetLayout(), set, 1, &l_DescriptorSet, 0, nullptr);
    }

    void VulkanCommandBuffer::PushConstants(ShaderStageFlags stages, uint32_t offset, uint32_t size, const void* data)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE || m_BoundPipeline == nullptr)
        {
            return;
        }

        vkCmdPushConstants(m_CommandBuffer, m_BoundPipeline->GetLayout(), ToVkShaderStageFlags(stages), offset, size, data);
    }

    void VulkanCommandBuffer::SetViewport(const Viewport& viewport)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VkViewport l_Viewport{};
        l_Viewport.x = viewport.X;
        l_Viewport.y = viewport.Y;
        l_Viewport.width = viewport.Width;
        l_Viewport.height = viewport.Height;
        l_Viewport.minDepth = viewport.MinDepth;
        l_Viewport.maxDepth = viewport.MaxDepth;

        vkCmdSetViewport(m_CommandBuffer, 0, 1, &l_Viewport);
    }

    void VulkanCommandBuffer::SetScissor(const ScissorRect& scissor)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VkRect2D l_Scissor{};
        l_Scissor.offset = { scissor.X, scissor.Y };
        l_Scissor.extent = { scissor.Width, scissor.Height };

        vkCmdSetScissor(m_CommandBuffer, 0, 1, &l_Scissor);
    }

    void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        if (m_CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
}