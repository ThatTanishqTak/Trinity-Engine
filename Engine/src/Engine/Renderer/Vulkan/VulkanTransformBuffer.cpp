#include "Engine/Renderer/Vulkan/VulkanTransformBuffer.h"

#include "Engine/Renderer/Vulkan/VulkanDebugUtils.h"
#include "Engine/Renderer/Vulkan/VulkanDevice.h"
#include "Engine/Utilities/Utilities.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

namespace Engine
{
    void VulkanTransformBuffer::Initialize(VulkanDevice& device, uint32_t framesInFlight, uint32_t maxDraws)
    {
        Shutdown(device, nullptr);

        if (!device.GetDevice() || framesInFlight == 0 || maxDraws == 0)
        {
            return;
        }

        m_MaxDraws = maxDraws;
        m_FrameData.clear();
        m_FrameData.resize(framesInFlight);

        const VkDeviceSize l_BufferSize = static_cast<VkDeviceSize>(maxDraws) * sizeof(glm::mat4);
        const VulkanDebugUtils* l_DebugUtils = device.GetDebugUtils();
        for (uint32_t l_FrameIndex = 0; l_FrameIndex < framesInFlight; ++l_FrameIndex)
        {
            FrameData& l_Frame = m_FrameData[l_FrameIndex];
            l_Frame.Buffer = VulkanResources::CreateBuffer(device, l_BufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (l_DebugUtils && l_Frame.Buffer.Buffer)
            {
                const std::string l_BufferName = "TransformBuffer_Frame" + std::to_string(l_FrameIndex);
                l_DebugUtils->SetObjectName(device.GetDevice(), VK_OBJECT_TYPE_BUFFER, static_cast<uint64_t>(l_Frame.Buffer.Buffer), l_BufferName.c_str());
            }

            void* l_Mapped = nullptr;
            Utilities::VulkanUtilities::VKCheckStrict(vkMapMemory(device.GetDevice(), l_Frame.Buffer.Memory, 0, l_BufferSize, 0, &l_Mapped), "vkMapMemory (transform buffer)");
            l_Frame.Mapped = l_Mapped;
            l_Frame.Count = 0;
        }
    }

    void VulkanTransformBuffer::Shutdown(VulkanDevice& device, DeletionQueue* deletionQueue)
    {
        if (m_FrameData.empty())
        {
            m_MaxDraws = 0;

            return;
        }

        const uint32_t l_FrameCount = static_cast<uint32_t>(m_FrameData.size());
        for (uint32_t l_FrameIndex = 0; l_FrameIndex < l_FrameCount; ++l_FrameIndex)
        {
            FrameData& l_Frame = m_FrameData[l_FrameIndex];
            if (l_Frame.Buffer.Buffer)
            {
                if (l_Frame.Mapped && l_Frame.Buffer.Memory)
                {
                    vkUnmapMemory(device.GetDevice(), l_Frame.Buffer.Memory);
                }

                l_Frame.Mapped = nullptr;

                if (deletionQueue)
                {
                    VulkanResources::BufferResource l_Buffer = l_Frame.Buffer;
                    l_Frame.Buffer = {};
                    deletionQueue->Push(l_FrameIndex, [&device, l_Buffer]() mutable
                    {
                        VulkanResources::DestroyBuffer(device, l_Buffer);
                    });
                }
                else
                {
                    VulkanResources::DestroyBuffer(device, l_Frame.Buffer);
                }
            }

            l_Frame.Count = 0;
        }

        m_FrameData.clear();
        m_MaxDraws = 0;
    }

    void VulkanTransformBuffer::BeginFrame(uint32_t frameIndex)
    {
        if (frameIndex >= m_FrameData.size())
        {
            TR_CORE_CRITICAL("VulkanTransformBuffer::BeginFrame out of range (frameIndex = {}, frames = {})", frameIndex, (uint32_t)m_FrameData.size());

            return;
        }

        m_FrameData[frameIndex].Count = 0;
    }

    uint32_t VulkanTransformBuffer::PushTransform(uint32_t frameIndex, const glm::mat4& transform)
    {
        if (frameIndex >= m_FrameData.size())
        {
            TR_CORE_CRITICAL("VulkanTransformBuffer::PushTransform out of range (frameIndex = {}, frames = {})", frameIndex, (uint32_t)m_FrameData.size());

            return 0;
        }

        FrameData& l_Frame = m_FrameData[frameIndex];
        assert(l_Frame.Count < m_MaxDraws && "VulkanTransformBuffer exceeded max draws");
        if (l_Frame.Count >= m_MaxDraws)
        {
            return l_Frame.Count;
        }

        const size_t l_Offset = static_cast<size_t>(l_Frame.Count) * sizeof(glm::mat4);
        std::memcpy(static_cast<std::byte*>(l_Frame.Mapped) + l_Offset, &transform, sizeof(glm::mat4));
        const uint32_t l_Index = l_Frame.Count;
        ++l_Frame.Count;

        return l_Index;
    }

    VkBuffer VulkanTransformBuffer::GetBuffer(uint32_t frameIndex) const
    {
        if (frameIndex >= m_FrameData.size())
        {
            TR_CORE_CRITICAL("VulkanTransformBuffer::GetBuffer out of range (frameIndex = {}, frames = {})", frameIndex, (uint32_t)m_FrameData.size());
            
            return VK_NULL_HANDLE;
        }

        return m_FrameData[frameIndex].Buffer.Buffer;
    }

    VkDeviceSize VulkanTransformBuffer::GetByteSize(uint32_t frameIndex) const
    {
        if (frameIndex >= m_FrameData.size())
        {
            TR_CORE_CRITICAL("VulkanTransformBuffer::GetByteSize out of range (frameIndex = {}, frames = {})", frameIndex, (uint32_t)m_FrameData.size());

            return 0;
        }

        return static_cast<VkDeviceSize>(m_FrameData[frameIndex].Count) * sizeof(glm::mat4);
    }
}