#include "Trinity/Renderer/Vulkan/VulkanDebug.h"

#include "Trinity/Utilities/Log.h"

namespace Trinity
{
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        switch (messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                TR_CORE_TRACE("[Vulkan] {}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                TR_CORE_DEBUG("[Vulkan] {}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                TR_CORE_WARN("[Vulkan] {}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                TR_CORE_ERROR("[Vulkan] {}", pCallbackData->pMessage);
                break;
            default:
                break;
        }

        return VK_FALSE;
    }

    void VulkanDebug::PopulateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;
    }

    void VulkanDebug::Initialize(VkInstance instance)
    {
        m_Instance = instance;

        VkDebugUtilsMessengerCreateInfoEXT l_CreateInfo{};
        PopulateCreateInfo(l_CreateInfo);

        auto a_CreateFunction = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

        if (a_CreateFunction != nullptr)
        {
            a_CreateFunction(instance, &l_CreateInfo, nullptr, &m_DebugMessenger);
            TR_CORE_INFO("Vulkan debug messenger created.");
        }
        else
        {
            TR_CORE_WARN("vkCreateDebugUtilsMessengerEXT not available.");
        }
    }

    void VulkanDebug::Shutdown()
    {
        if (m_DebugMessenger != VK_NULL_HANDLE && m_Instance != VK_NULL_HANDLE)
        {
            auto a_DestroyFunction = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));

            if (a_DestroyFunction != nullptr)
            {
                a_DestroyFunction(m_Instance, m_DebugMessenger, nullptr);
            }

            m_DebugMessenger = VK_NULL_HANDLE;
        }
    }
}