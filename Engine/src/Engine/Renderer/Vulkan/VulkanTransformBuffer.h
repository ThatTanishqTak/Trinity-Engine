#pragma once

#include "Engine/Renderer/DeletionQueue.h"
#include "Engine/Renderer/Vulkan/VulkanResources.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace Engine
{
    class VulkanDevice;

    class VulkanTransformBuffer
    {
    public:
        VulkanTransformBuffer() = default;
        ~VulkanTransformBuffer() = default;

        VulkanTransformBuffer(const VulkanTransformBuffer&) = delete;
        VulkanTransformBuffer& operator=(const VulkanTransformBuffer&) = delete;
        VulkanTransformBuffer(VulkanTransformBuffer&&) = delete;
        VulkanTransformBuffer& operator=(VulkanTransformBuffer&&) = delete;

        void Initialize(VulkanDevice& device, uint32_t framesInFlight, uint32_t maxDraws);
        void Shutdown(VulkanDevice& device, DeletionQueue* deletionQueue);

        void BeginFrame(uint32_t frameIndex);
        uint32_t PushTransform(uint32_t frameIndex, const glm::mat4& transform);

        VkBuffer GetBuffer(uint32_t frameIndex) const;
        VkDeviceSize GetByteSize(uint32_t frameIndex) const;

    private:
        struct FrameData
        {
            VulkanResources::BufferResource Buffer{};
            void* Mapped = nullptr;
            uint32_t Count = 0;
        };

        std::vector<FrameData> m_FrameData;
        uint32_t m_MaxDraws = 0;
    };
}