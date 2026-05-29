#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#include <Trinity/Renderer/Backends/Vulkan/VulkanAllocator.h>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    VulkanAllocator::~VulkanAllocator()
    {
        Shutdown();
    }

    bool VulkanAllocator::Initialize(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
    {
        VmaAllocatorCreateInfo l_AllocatorCreateInfo{};
        l_AllocatorCreateInfo.instance = instance;
        l_AllocatorCreateInfo.physicalDevice = physicalDevice;
        l_AllocatorCreateInfo.device = device;
        l_AllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        VkResult l_Result = vmaCreateAllocator(&l_AllocatorCreateInfo, &m_Allocator);
        if (l_Result != VK_SUCCESS)
        {
            TR_CORE_CRITICAL("VulkanAllocator: vmaCreateAllocator failed ({})", static_cast<int>(l_Result));
            return false;
        }

        TR_CORE_INFO("VulkanAllocator: created");
        return true;
    }

    void VulkanAllocator::Shutdown()
    {
        if (m_Allocator != VK_NULL_HANDLE)
        {
            LogStats();
            vmaDestroyAllocator(m_Allocator);
            m_Allocator = VK_NULL_HANDLE;
        }
    }

    void VulkanAllocator::LogStats() const
    {
        if (m_Allocator == VK_NULL_HANDLE)
        {
            return;
        }

        VmaTotalStatistics l_Stats{};
        vmaCalculateStatistics(m_Allocator, &l_Stats);

        const uint32_t l_AllocationCount = l_Stats.total.statistics.allocationCount;
        const uint64_t l_UsedBytes = l_Stats.total.statistics.allocationBytes;

        if (l_AllocationCount == 0)
        {
            TR_CORE_INFO("VulkanAllocator: no live allocations");
        }
        else
        {
            TR_CORE_WARN("VulkanAllocator: {} live allocations ({} bytes) at shutdown", l_AllocationCount, l_UsedBytes);
        }
    }
}