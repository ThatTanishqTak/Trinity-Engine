#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace Trinity
{
    namespace Utilities
    {
        class VulkanUtilities
        {
        public:
            static void VKCheck(VkResult result, const char* what);
            
            static VKAPI_ATTR VkBool32 VKAPI_CALL VKDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

            static bool HasExtension(const std::vector<const char*>& list, const char* name);
        };
    }
}