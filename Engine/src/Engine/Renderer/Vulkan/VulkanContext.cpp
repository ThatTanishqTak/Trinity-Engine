#include "Engine/Renderer/Vulkan/VulkanContext.h"

#include "Engine/Utilities/Utilities.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <vector>
#include <cstdlib>

namespace
{
#ifdef NDEBUG
    constexpr bool s_EnableValidationLayers = false;
#else
    constexpr bool s_EnableValidationLayers = true;
#endif

    const std::vector<const char*> s_ValidationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    bool CheckValidationLayerSupport()
    {
        uint32_t l_LayerCount = 0;
        vkEnumerateInstanceLayerProperties(&l_LayerCount, nullptr);

        std::vector<VkLayerProperties> l_Available(l_LayerCount);
        vkEnumerateInstanceLayerProperties(&l_LayerCount, l_Available.data());

        for (const char* it_LayerName : s_ValidationLayers)
        {
            bool l_Found = false;
            for (const auto& it_Prop : l_Available)
            {
                if (std::strcmp(it_LayerName, it_Prop.layerName) == 0)
                {
                    l_Found = true;
                    break;
                }
            }

            if (!l_Found)
            {
                return false;
            }
        }

        return true;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        (void)messageType;
        (void)pUserData;

        if (!pCallbackData || !pCallbackData->pMessage)
        {
            return VK_FALSE;
        }

        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            TR_CORE_ERROR("[Vulkan] {}", pCallbackData->pMessage);
        }
        else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            TR_CORE_WARN("[Vulkan] {}", pCallbackData->pMessage);
        }
        else
        {
            TR_CORE_TRACE("[Vulkan] {}", pCallbackData->pMessage);
        }

        return VK_FALSE;
    }

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto a_Function = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (a_Function != nullptr)
        {
            return a_Function(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }

        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
    {
        auto a_Function = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (a_Function != nullptr)
        {
            a_Function(instance, debugMessenger, pAllocator);
        }
    }

    VkDebugUtilsMessengerCreateInfoEXT MakeDebugMessengerCreateInfo()
    {
        VkDebugUtilsMessengerCreateInfoEXT l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        l_CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        l_CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        l_CreateInfo.pfnUserCallback = DebugCallback;

        return l_CreateInfo;
    }
}

namespace Engine
{
    void VulkanContext::Initialize(GLFWwindow* nativeWindow)
    {
        m_NativeWindow = nativeWindow;

        if (!m_NativeWindow)
        {
            TR_CORE_CRITICAL("VulkanContext: Native window handle is null (expected GLFWwindow*).");

            std::abort();
        }

        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
    }

    void VulkanContext::Shutdown()
    {
        if (m_UseValidationLayers && m_DebugMessenger)
        {
            DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
            m_DebugMessenger = VK_NULL_HANDLE;
        }

        if (m_Surface)
        {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }

        if (m_Instance)
        {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
        }

        m_NativeWindow = nullptr;
        m_UseValidationLayers = false;
    }

    void VulkanContext::CreateInstance()
    {
        if (s_EnableValidationLayers && !CheckValidationLayerSupport())
        {
            TR_CORE_WARN("Validation layers requested but not available. Continuing without validation layers.");
        }

        VkApplicationInfo l_App{};
        l_App.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        l_App.pApplicationName = "PhysicsEngine";
        l_App.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        l_App.pEngineName = "Engine";
        l_App.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        l_App.apiVersion = VK_API_VERSION_1_3;

        uint32_t l_GLFWExtCount = 0;
        const char** l_GLFWExts = glfwGetRequiredInstanceExtensions(&l_GLFWExtCount);
        std::vector<const char*> l_Extensions(l_GLFWExts, l_GLFWExts + l_GLFWExtCount);

        m_UseValidationLayers = s_EnableValidationLayers && CheckValidationLayerSupport();
        if (m_UseValidationLayers)
        {
            l_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        VkInstanceCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        l_Info.pApplicationInfo = &l_App;
        l_Info.enabledExtensionCount = static_cast<uint32_t>(l_Extensions.size());
        l_Info.ppEnabledExtensionNames = l_Extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT l_DebugInfo{};
        if (m_UseValidationLayers)
        {
            l_Info.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
            l_Info.ppEnabledLayerNames = s_ValidationLayers.data();

            l_DebugInfo = MakeDebugMessengerCreateInfo();
            l_Info.pNext = &l_DebugInfo;
        }

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateInstance(&l_Info, nullptr, &m_Instance), "vkCreateInstance");
    }

    void VulkanContext::SetupDebugMessenger()
    {
        if (!m_UseValidationLayers)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT l_Info = MakeDebugMessengerCreateInfo();
        VkResult l_Result = CreateDebugUtilsMessengerEXT(m_Instance, &l_Info, nullptr, &m_DebugMessenger);
        if (l_Result != VK_SUCCESS)
        {
            TR_CORE_WARN("Failed to create debug messenger. Vulkan will be less chatty.");
        }
    }

    void VulkanContext::CreateSurface()
    {
        Engine::Utilities::VulkanUtilities::VKCheckStrict(glfwCreateWindowSurface(m_Instance, m_NativeWindow, nullptr, &m_Surface), "glfwCreateWindowSurface");
    }

    const std::vector<const char*>& VulkanContext::GetValidationLayers()
    {
        return s_ValidationLayers;
    }
}