#include <Trinity/Renderer/Backends/Vulkan/VulkanInstance.h>

#include <cstring>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    static const std::vector<const char*> s_ValidationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
    {
        if (data == nullptr || data->pMessage == nullptr)
        {
            return VK_FALSE;
        }

        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            TR_CORE_ERROR("[Vulkan] {}", data->pMessage);
        }
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            TR_CORE_WARN("[Vulkan] {}", data->pMessage);
        }
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            TR_CORE_INFO("[Vulkan] {}", data->pMessage);
        }
        else
        {
            TR_CORE_TRACE("[Vulkan] {}", data->pMessage);
        }

        return VK_FALSE;
    }

    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* createInfo, VkDebugUtilsMessengerEXT* outMessenger)
    {
        auto a_Function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        if (a_Function == nullptr)
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }

        return a_Function(instance, createInfo, nullptr, outMessenger);
    }

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger)
    {
        auto a_Function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (a_Function != nullptr)
        {
            a_Function(instance, messenger, nullptr);
        }
    }

    static VkDebugUtilsMessengerCreateInfoEXT MakeMessengerCreateInfo()
    {
        VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo{};
        l_DebugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        l_DebugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        l_DebugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        l_DebugCreateInfo.pfnUserCallback = DebugCallback;

        return l_DebugCreateInfo;
    }

    VulkanInstance::~VulkanInstance()
    {
        Shutdown();
    }

    bool VulkanInstance::Initialize(const std::string& applicationName, bool enableValidation)
    {
        m_ValidationEnabled = enableValidation;

        if (m_ValidationEnabled && !CheckLayerSupport(s_ValidationLayers))
        {
            TR_CORE_WARN("VulkanInstance: validation layers requested but not available, disabling");
            m_ValidationEnabled = false;
        }

        if (!CreateInstance(applicationName))
        {
            return false;
        }

        if (m_ValidationEnabled && !SetupDebugMessenger())
        {
            TR_CORE_WARN("VulkanInstance: debug messenger setup failed");
        }

        TR_CORE_INFO("VulkanInstance: created (validation {})", m_ValidationEnabled ? "enabled" : "disabled");
        return true;
    }

    void VulkanInstance::Shutdown()
    {
        if (m_DebugMessenger != VK_NULL_HANDLE)
        {
            DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger);
            m_DebugMessenger = VK_NULL_HANDLE;
        }

        if (m_Instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
        }
    }

    bool VulkanInstance::CreateInstance(const std::string& applicationName)
    {
        VkApplicationInfo l_ApplicationInfo{};
        l_ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        l_ApplicationInfo.pApplicationName = applicationName.c_str();
        l_ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        l_ApplicationInfo.pEngineName = "Trinity";
        l_ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        l_ApplicationInfo.apiVersion = VK_API_VERSION_1_3;

        std::vector<const char*> l_Extensions = GetRequiredExtensions();
        std::vector<const char*> l_Layers = m_ValidationEnabled ? s_ValidationLayers : std::vector<const char*>{};

        VkInstanceCreateInfo l_InstanceCreateInfo{};
        l_InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        l_InstanceCreateInfo.pApplicationInfo = &l_ApplicationInfo;
        l_InstanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(l_Extensions.size());
        l_InstanceCreateInfo.ppEnabledExtensionNames = l_Extensions.data();
        l_InstanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(l_Layers.size());
        l_InstanceCreateInfo.ppEnabledLayerNames = l_Layers.data();

        VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo = MakeMessengerCreateInfo();
        if (m_ValidationEnabled)
        {
            l_InstanceCreateInfo.pNext = &l_DebugCreateInfo;
        }

        VkResult l_Result = vkCreateInstance(&l_InstanceCreateInfo, nullptr, &m_Instance);
        if (l_Result != VK_SUCCESS)
        {
            TR_CORE_CRITICAL("VulkanInstance: vkCreateInstance failed ({})", static_cast<int>(l_Result));
            return false;
        }

        return true;
    }

    bool VulkanInstance::SetupDebugMessenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo = MakeMessengerCreateInfo();
        VkResult l_Result = CreateDebugUtilsMessengerEXT(m_Instance, &l_DebugCreateInfo, &m_DebugMessenger);

        return l_Result == VK_SUCCESS;
    }

    std::vector<const char*> VulkanInstance::GetRequiredExtensions() const
    {
        std::vector<const char*> l_Extensions;

        l_Extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(TRINITY_PLATFORM_WINDOWS)
        l_Extensions.push_back("VK_KHR_win32_surface");
#elif defined(TRINITY_PLATFORM_LINUX)
        l_Extensions.push_back("VK_KHR_xlib_surface");
        l_Extensions.push_back("VK_KHR_wayland_surface");
#elif defined(TRINITY_PLATFORM_MACOS)
        l_Extensions.push_back("VK_EXT_metal_surface");
        l_Extensions.push_back("VK_KHR_portability_enumeration");
#endif

        if (m_ValidationEnabled)
        {
            l_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return l_Extensions;
    }

    std::vector<const char*> VulkanInstance::GetRequiredLayers() const
    {
        if (m_ValidationEnabled)
        {
            return s_ValidationLayers;
        }

        return {};
    }

    bool VulkanInstance::CheckLayerSupport(const std::vector<const char*>& requestedLayers) const
    {
        uint32_t l_LayerCount = 0;
        vkEnumerateInstanceLayerProperties(&l_LayerCount, nullptr);

        std::vector<VkLayerProperties> l_Available(l_LayerCount);
        vkEnumerateInstanceLayerProperties(&l_LayerCount, l_Available.data());

        for (const char* l_Requested : requestedLayers)
        {
            bool l_Found = false;
            for (const VkLayerProperties& l_Layer : l_Available)
            {
                if (std::strcmp(l_Layer.layerName, l_Requested) == 0)
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
}