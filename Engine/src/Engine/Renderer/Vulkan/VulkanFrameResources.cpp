#include "Engine/Renderer/Vulkan/VulkanFrameResources.h"

#include "Engine/Renderer/Vulkan/VulkanDescriptors.h"
#include "Engine/Renderer/Vulkan/VulkanResources.h"
#include "Engine/Renderer/Camera.h"
#include "Engine/Utilities/Utilities.h"

#include <glm/glm.hpp>

#include <cstring>

namespace Engine
{
    namespace
    {
        struct UniformBufferObject
        {
            glm::mat4 MVP;
        };
    }

    void VulkanFrameResources::Initialize(VkDevice device, VulkanResources* resources, VulkanDescriptors* descriptors, uint32_t maxFramesInFlight)
    {
        m_Device = device;
        m_Resources = resources;
        m_Descriptors = descriptors;
        m_MaxFramesInFlight = maxFramesInFlight;
    }

    void VulkanFrameResources::Shutdown()
    {
        DestroyUniformBuffers();
        DestroyDescriptors();

        m_UniformBuffersMapped.clear();
        m_UniformBuffersMemory.clear();
        m_UniformBuffers.clear();
        m_DescriptorSets.clear();

        m_Device = VK_NULL_HANDLE;
        m_Resources = nullptr;
        m_Descriptors = nullptr;
        m_MaxFramesInFlight = 0;
    }

    void VulkanFrameResources::CreateUniformBuffers()
    {
        m_UniformBuffers.resize(m_MaxFramesInFlight);
        m_UniformBuffersMemory.resize(m_MaxFramesInFlight);
        m_UniformBuffersMapped.resize(m_MaxFramesInFlight);

        VkDeviceSize l_BufferSize = sizeof(UniformBufferObject);

        for (uint32_t i = 0; i < m_MaxFramesInFlight; ++i)
        {
            m_Resources->CreateBuffer(l_BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_UniformBuffers[i], m_UniformBuffersMemory[i]);

            void* l_Mapped = nullptr;
            Engine::Utilities::VulkanUtilities::VKCheckStrict(vkMapMemory(m_Device, m_UniformBuffersMemory[i], 0, l_BufferSize, 0, &l_Mapped), "vkMapMemory(UniformBuffer)");

            m_UniformBuffersMapped[i] = l_Mapped;
        }
    }

    void VulkanFrameResources::DestroyUniformBuffers()
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_UniformBuffers.size()); ++i)
        {
            if (m_UniformBuffersMapped[i])
            {
                vkUnmapMemory(m_Device, m_UniformBuffersMemory[i]);
                m_UniformBuffersMapped[i] = nullptr;
            }

            m_Resources->DestroyBuffer(m_UniformBuffers[i], m_UniformBuffersMemory[i]);
        }

        m_UniformBuffers.clear();
        m_UniformBuffersMemory.clear();
        m_UniformBuffersMapped.clear();
    }

    void VulkanFrameResources::UpdateUniformBuffer(uint32_t frameIndex, VkExtent2D extent, const Render::Camera& camera)
    {
        (void)extent;

        UniformBufferObject l_Ubo{};

        glm::mat4 l_Model = glm::mat4(1.0f);
        glm::mat4 l_View = camera.GetViewMatrix();
        glm::mat4 l_Proj = camera.GetProjectionMatrix();

        // Vulkan clip space has inverted Y compared to OpenGL.
        l_Proj[1][1] *= -1.0f;

        l_Ubo.MVP = l_Proj * l_View * l_Model;

        std::memcpy(m_UniformBuffersMapped[frameIndex], &l_Ubo, sizeof(UniformBufferObject));
    }

    void VulkanFrameResources::CreateDescriptors()
    {
        VkDescriptorSetLayoutBinding l_UboBinding{};
        l_UboBinding.binding = 0;
        l_UboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_UboBinding.descriptorCount = 1;
        l_UboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        l_UboBinding.pImmutableSamplers = nullptr;

        m_Descriptors->CreateLayout({ l_UboBinding });

        VkDescriptorPoolSize l_PoolSize{};
        l_PoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_PoolSize.descriptorCount = m_MaxFramesInFlight;

        m_Descriptors->CreatePool(m_MaxFramesInFlight, { l_PoolSize });

        m_Descriptors->AllocateSets(m_MaxFramesInFlight, m_DescriptorSets);

        for (uint32_t i = 0; i < m_MaxFramesInFlight; ++i)
        {
            m_Descriptors->UpdateBuffer(m_DescriptorSets[i], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_UniformBuffers[i], sizeof(UniformBufferObject), 0);
        }
    }

    void VulkanFrameResources::DestroyDescriptors()
    {
        m_DescriptorSets.clear();
        if (m_Descriptors)
        {
            m_Descriptors->DestroyPool();
            m_Descriptors->DestroyLayout();
        }
    }

    VkBuffer VulkanFrameResources::GetUniformBuffer(uint32_t frameIndex) const
    {
        return m_UniformBuffers[frameIndex];
    }

    VkDeviceMemory VulkanFrameResources::GetUniformBufferMemory(uint32_t frameIndex) const
    {
        return m_UniformBuffersMemory[frameIndex];
    }

    void* VulkanFrameResources::GetUniformBufferMapped(uint32_t frameIndex) const
    {
        return m_UniformBuffersMapped[frameIndex];
    }

    VkDescriptorSet VulkanFrameResources::GetDescriptorSet(uint32_t frameIndex) const
    {
        return m_DescriptorSets[frameIndex];
    }
}