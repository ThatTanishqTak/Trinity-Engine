#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4189) // local variable is initialized but not referenced
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

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