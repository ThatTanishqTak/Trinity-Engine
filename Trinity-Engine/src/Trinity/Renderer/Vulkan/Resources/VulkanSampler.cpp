#include "Trinity/Renderer/Vulkan/Resources/VulkanSampler.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

namespace Trinity
{
    VulkanSampler::VulkanSampler(VkDevice device, const SamplerSpecification& specification) : m_Device(device)
    {
        m_Specification = specification;

        VkSamplerCreateInfo l_SamplerInfo{};
        l_SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        l_SamplerInfo.magFilter = VulkanUtilities::ToVkFilter(specification.MagFilter);
        l_SamplerInfo.minFilter = VulkanUtilities::ToVkFilter(specification.MinFilter);
        l_SamplerInfo.mipmapMode = VulkanUtilities::ToVkMipmapMode(specification.MipmapMode);
        l_SamplerInfo.addressModeU = VulkanUtilities::ToVkAddressMode(specification.AddressModeU);
        l_SamplerInfo.addressModeV = VulkanUtilities::ToVkAddressMode(specification.AddressModeV);
        l_SamplerInfo.addressModeW = VulkanUtilities::ToVkAddressMode(specification.AddressModeW);
        l_SamplerInfo.mipLodBias = specification.MipLodBias;
        l_SamplerInfo.anisotropyEnable = specification.AnisotropyEnable ? VK_TRUE : VK_FALSE;
        l_SamplerInfo.maxAnisotropy = specification.MaxAnisotropy;
        l_SamplerInfo.compareEnable = VK_FALSE;
        l_SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        l_SamplerInfo.minLod = specification.MinLod;
        l_SamplerInfo.maxLod = specification.MaxLod;
        l_SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        l_SamplerInfo.unnormalizedCoordinates = VK_FALSE;

        VulkanUtilities::VKCheck(vkCreateSampler(m_Device, &l_SamplerInfo, nullptr, &m_Sampler), "Failed vkCreateSampler");
    }

    VulkanSampler::~VulkanSampler()
    {
        if (m_Sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_Device, m_Sampler, nullptr);
        }
    }
}