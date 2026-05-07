#pragma once

#include "Trinity/Renderer/Resources/DescriptorSetLayout.h"

#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanDescriptorSetLayout final : public DescriptorSetLayout
    {
    public:
        VulkanDescriptorSetLayout(VkDevice device, const DescriptorSetLayoutSpecification& specification);
        ~VulkanDescriptorSetLayout() override;

        uint64_t GetOpaqueHandle() const override { return reinterpret_cast<uint64_t>(m_Layout); }

        VkDescriptorSetLayout GetVkLayout() const { return m_Layout; }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    };
}