#include "Trinity/Renderer/Vulkan/Resources/VulkanDescriptorSetLayout.h"

#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

#include <vector>

namespace Trinity
{
    namespace
    {
        VkDescriptorType ToVkDescriptorType(DescriptorBindingType type)
        {
            switch (type)
            {
                case DescriptorBindingType::UniformBuffer:
                    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                case DescriptorBindingType::StorageBuffer:
                    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                case DescriptorBindingType::SampledImage:
                    return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                case DescriptorBindingType::StorageImage:
                    return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                case DescriptorBindingType::Sampler:
                    return VK_DESCRIPTOR_TYPE_SAMPLER;
                case DescriptorBindingType::CombinedImageSampler:
                    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                case DescriptorBindingType::UniformBufferDynamic:
                    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                case DescriptorBindingType::StorageBufferDynamic:
                    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                default:
                    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
        }

        VkDescriptorBindingFlags ToVkBindingFlags(DescriptorBindingFlags flags)
        {
            VkDescriptorBindingFlags l_Flags = 0;
            if (flags & DescriptorBindingFlags::PartiallyBound)
            {
                l_Flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
            }

            if (flags & DescriptorBindingFlags::UpdateAfterBind)
            {
                l_Flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
            }

            if (flags & DescriptorBindingFlags::VariableDescriptorCount)
            {
                l_Flags |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
            }

            return l_Flags;
        }
    }

    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice device, const DescriptorSetLayoutSpecification& specification) : m_Device(device)
    {
        m_Specification = specification;

        std::vector<VkDescriptorSetLayoutBinding> l_VkBindings;
        l_VkBindings.reserve(specification.Bindings.size());

        std::vector<VkDescriptorBindingFlags> l_VkBindingFlags;
        l_VkBindingFlags.reserve(specification.Bindings.size());

        bool l_AnyBindlessFlags = false;
        bool l_AnyUpdateAfterBind = false;

        for (const auto& it_Binding : specification.Bindings)
        {
            VkDescriptorSetLayoutBinding l_VkBinding{};
            l_VkBinding.binding = it_Binding.Binding;
            l_VkBinding.descriptorType = ToVkDescriptorType(it_Binding.Type);
            l_VkBinding.descriptorCount = it_Binding.Count;
            l_VkBinding.stageFlags = VulkanUtilities::ToVkShaderStage(it_Binding.Stage);
            l_VkBinding.pImmutableSamplers = nullptr;
            l_VkBindings.push_back(l_VkBinding);

            const VkDescriptorBindingFlags l_Flags = ToVkBindingFlags(it_Binding.Flags);
            l_VkBindingFlags.push_back(l_Flags);

            if (l_Flags != 0)
            {
                l_AnyBindlessFlags = true;
            }

            if (it_Binding.Flags & DescriptorBindingFlags::UpdateAfterBind)
            {
                l_AnyUpdateAfterBind = true;
            }
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfo l_FlagsInfo{};
        l_FlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        l_FlagsInfo.bindingCount = static_cast<uint32_t>(l_VkBindingFlags.size());
        l_FlagsInfo.pBindingFlags = l_VkBindingFlags.empty() ? nullptr : l_VkBindingFlags.data();

        VkDescriptorSetLayoutCreateInfo l_LayoutInfo{};
        l_LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        l_LayoutInfo.bindingCount = static_cast<uint32_t>(l_VkBindings.size());
        l_LayoutInfo.pBindings = l_VkBindings.empty() ? nullptr : l_VkBindings.data();

        if (l_AnyBindlessFlags)
        {
            l_LayoutInfo.pNext = &l_FlagsInfo;
        }

        if (l_AnyUpdateAfterBind)
        {
            l_LayoutInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        }

        VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device, &l_LayoutInfo, nullptr, &m_Layout), "Failed vkCreateDescriptorSetLayout");
    }

    VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
    {
        if (m_Layout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
        }
    }
}