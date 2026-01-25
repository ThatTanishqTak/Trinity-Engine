#include "Engine/Renderer/Vulkan/VulkanDescriptors.h"

#include "Engine/Renderer/Vulkan/VulkanDevice.h"
#include "Engine/Utilities/Utilities.h"

namespace Engine
{
    void VulkanDescriptors::Initialize(VulkanDevice& device, uint32_t framesInFlight)
    {
        Shutdown(device);

        m_Device = &device;

        if (!device.GetDevice() || framesInFlight == 0)
        {
            return;
        }

        // Minimal layout: a single uniform buffer at set 0, binding 0.
        VkDescriptorSetLayoutBinding l_LayoutBinding{};
        l_LayoutBinding.binding = 0;
        l_LayoutBinding.descriptorCount = 1;
        l_LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_LayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo l_LayoutCreateInfo{};
        l_LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        l_LayoutCreateInfo.bindingCount = 1;
        l_LayoutCreateInfo.pBindings = &l_LayoutBinding;

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateDescriptorSetLayout(device.GetDevice(), &l_LayoutCreateInfo, nullptr, &m_Layout), "vkCreateDescriptorSetLayout");

        VkDescriptorPoolSize l_PoolSize{};
        l_PoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        l_PoolSize.descriptorCount = framesInFlight;

        VkDescriptorPoolCreateInfo l_PoolCreateInfo{};
        l_PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_PoolCreateInfo.poolSizeCount = 1;
        l_PoolCreateInfo.pPoolSizes = &l_PoolSize;
        l_PoolCreateInfo.maxSets = framesInFlight;

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateDescriptorPool(device.GetDevice(), &l_PoolCreateInfo, nullptr, &m_Pool), "vkCreateDescriptorPool");

        m_DescriptorSets.assign(framesInFlight, VK_NULL_HANDLE);
        std::vector<VkDescriptorSetLayout> l_Layouts(framesInFlight, m_Layout);

        VkDescriptorSetAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_AllocateInfo.descriptorPool = m_Pool;
        l_AllocateInfo.descriptorSetCount = framesInFlight;
        l_AllocateInfo.pSetLayouts = l_Layouts.data();

        Utilities::VulkanUtilities::VKCheckStrict(vkAllocateDescriptorSets(device.GetDevice(), &l_AllocateInfo, m_DescriptorSets.data()), "vkAllocateDescriptorSets");
    }

    void VulkanDescriptors::Shutdown(VulkanDevice& device)
    {
        Shutdown(device, {});
    }

    void VulkanDescriptors::Shutdown(VulkanDevice& device, const std::function<void(std::function<void()>&&)>& submitResourceFree)
    {
        if (!device.GetDevice())
        {
            m_Pool = VK_NULL_HANDLE;
            m_Layout = VK_NULL_HANDLE;
            m_DescriptorSets.clear();
            m_Device = nullptr;

            return;
        }

        VkDescriptorPool l_Pool = m_Pool;
        VkDescriptorSetLayout l_Layout = m_Layout;
        VulkanDevice* l_Device = &device;

        m_Pool = VK_NULL_HANDLE;
        m_Layout = VK_NULL_HANDLE;
        m_DescriptorSets.clear();
        m_Device = nullptr;

        if (submitResourceFree)
        {
            submitResourceFree([l_Device, l_Pool, l_Layout]() mutable
            {
                if (!l_Device || !l_Device->GetDevice())
                {
                    return;
                }

                if (l_Pool != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorPool(l_Device->GetDevice(), l_Pool, nullptr);
                }

                if (l_Layout != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorSetLayout(l_Device->GetDevice(), l_Layout, nullptr);
                }
            });

            return;
        }

        if (l_Pool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device.GetDevice(), l_Pool, nullptr);
        }

        if (l_Layout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device.GetDevice(), l_Layout, nullptr);
        }
    }

    VkDescriptorSet VulkanDescriptors::GetDescriptorSet(uint32_t frameIndex) const
    {
        if (frameIndex >= m_DescriptorSets.size())
        {
            return VK_NULL_HANDLE;
        }

        return m_DescriptorSets[frameIndex];
    }
}