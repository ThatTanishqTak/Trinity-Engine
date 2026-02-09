#include "Trinity/Utilities/VulkanUtilities.h"

#include "Trinity/Utilities/Log.h"

namespace Trinity
{
    namespace Utilities
    {
        void VulkanUtilities::VKCheck(VkResult result, const char* what)
        {
            if (result != VK_SUCCESS)
            {
                TR_CORE_CRITICAL("Vulkan failure: {} (VkResult = {})", what, (int)result);

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
    }
}