#include "Trinity/Renderer/Vulkan/VulkanAllocator.h"

#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"

namespace Trinity
{
    void VulkanAllocator::Initialize(const VulkanDevice& device)
    {
        VmaAllocatorCreateInfo l_AllocatorInfo{};
        l_AllocatorInfo.instance = device.GetInstance();
        l_AllocatorInfo.physicalDevice = device.GetPhysicalDevice();
        l_AllocatorInfo.device = device.GetDevice();
        l_AllocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        VulkanUtilities::VKCheck(vmaCreateAllocator(&l_AllocatorInfo, &m_Allocator), "Failed vmaCreateAllocator");

        TR_CORE_INFO("VMA allocator initialized.");
    }

    void VulkanAllocator::Shutdown()
    {
        if (m_Allocator != VK_NULL_HANDLE)
        {
            vmaDestroyAllocator(m_Allocator);
            m_Allocator = VK_NULL_HANDLE;
        }
    }
}