#include "Trinity/Utilities/VulkanUtilities.h"

#include "Trinity/Utilities/Log.h"

#include <string>

namespace Trinity
{
    namespace Utilities
    {
        void VulkanUtilities::VKCheck(VkResult result, const char* what)
        {
            if (result != VK_SUCCESS)
            {
                TR_CORE_CRITICAL("Vulkan failure: {} (VkResult = {})", what, std::to_string(result));

                std::abort();
            }
        }

        VKAPI_ATTR VkBool32 VKAPI_CALL VulkanUtilities::VKDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
        {
            switch (severity)
            {
                default:
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    TR_CORE_ERROR("[VULKAN]: {}", callbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    TR_CORE_WARN("[VULKAN]: {}", callbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                    TR_CORE_INFO("[VULKAN]: {}", callbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                    TR_CORE_TRACE("[VULKAN]: {}", callbackData->pMessage);
                    break;
            }

            return VK_FALSE;
        }

        bool VulkanUtilities::HasExtension(const std::vector<const char*>& list, const char* name)
        {
            return std::any_of(list.begin(), list.end(), [&](const char* s) {return s && name && std::strcmp(s, name) == 0; });
        }

        uint32_t VulkanUtilities::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
        {
            VkPhysicalDeviceMemoryProperties l_MemoryProperties{};
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &l_MemoryProperties);

            for (uint32_t it_Index = 0; it_Index < l_MemoryProperties.memoryTypeCount; ++it_Index)
            {
                if ((typeFilter & (1u << it_Index)) && (l_MemoryProperties.memoryTypes[it_Index].propertyFlags & properties) == properties)
                {
                    return it_Index;
                }
            }

            TR_CORE_CRITICAL("Failed to find suitable Vulkan memory type for scene viewport image");
            std::abort();
        }

        VkFormat VulkanUtilities::FindDepthFormat(VkPhysicalDevice physicalDevice)
        {
            const VkFormat l_Candidates[] =
            {
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT
            };

            for (VkFormat it_Format : l_Candidates)
            {
                VkFormatProperties l_Props{};
                vkGetPhysicalDeviceFormatProperties(physicalDevice, it_Format, &l_Props);

                if ((l_Props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
                {
                    return it_Format;
                }
            }

            TR_CORE_CRITICAL("Failed to find a supported depth format");
            std::abort();
        }
    }
}