#pragma once

#include "Trinity/Renderer/Resources/Sampler.h"

#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanSampler final : public Sampler
    {
    public:
        VulkanSampler(VkDevice device, const SamplerSpecification& specification);
        ~VulkanSampler() override;

        VkSampler GetSampler() const { return m_Sampler; }
    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;
    };
}