#include "Trinity/Renderer/Vulkan/VulkanContext.h"

#include "Trinity/Utilities/Log.h"
#include "Trinity/Utilities/VulkanUtilities.h"
#include "Trinity/Renderer/Vulkan/VulkanSurface.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <vector>


namespace Trinity
{
    void VulkanContext::Initialize(const NativeWindowHandle& nativeWindowHandle)
    {
        TR_CORE_TRACE("Initializing Vulkan Context");

        if (m_Instance != VK_NULL_HANDLE)
        {
            TR_CORE_WARN("VulkanContext::Initialize called while already initialized");

            return;
        }

        CreateInstance();
        SetupDebugMessenger();
        CreateSurface(nativeWindowHandle);

        TR_CORE_TRACE("Vulkan Context Initialized");
    }

    void VulkanContext::Shutdown()
    {
        TR_CORE_TRACE("Shutting Down Vulkan Context");

        if (m_Instance == VK_NULL_HANDLE)
        {
            TR_CORE_WARN("VulkanContext::Shutdown called while not initialized");

            return;
        }

        DestroySurface();
        DestroyDebugMessenger();
        DestroyInstance();

        TR_CORE_TRACE("Vulkan Context Shutdown Complete");
    }

    void VulkanContext::CreateInstance()
    {
        TR_CORE_TRACE("Creating Vulkan Instance");

        uint32_t l_LoaderVersion = VK_API_VERSION_1_0;
        auto l_EnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));

        if (!l_EnumerateInstanceVersion)
        {
            TR_CORE_CRITICAL("Vulkan loader/runtime is too old (vkEnumerateInstanceVersion missing). Vulkan 1.3 is required.");

            std::abort();
        }

        Utilities::VulkanUtilities::VKCheck(l_EnumerateInstanceVersion(&l_LoaderVersion), "Failed vkEnumerateInstanceVersion");

        if (l_LoaderVersion < VK_API_VERSION_1_3)
        {
            TR_CORE_CRITICAL("Vulkan 1.3 is required (found loader/runtime API {}.{}.{})", VK_VERSION_MAJOR(l_LoaderVersion), VK_VERSION_MINOR(l_LoaderVersion),
                VK_VERSION_PATCH(l_LoaderVersion));

            std::abort();
        }

        m_RequiredExtensions = GetRequiredExtensions();
        m_RequiredLayers = GetRequiredLayers();

        VkApplicationInfo l_AppInfo{};
        l_AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        l_AppInfo.apiVersion = VK_API_VERSION_1_3;
        l_AppInfo.pApplicationName = "Trinity-Application";
        l_AppInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        l_AppInfo.pEngineName = "Trinity-Engine";
        l_AppInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);

        VkInstanceCreateInfo l_InstanceCreateInfo{};
        l_InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        l_InstanceCreateInfo.pApplicationInfo = &l_AppInfo;
        l_InstanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_RequiredExtensions.size());
        l_InstanceCreateInfo.ppEnabledExtensionNames = m_RequiredExtensions.data();
        l_InstanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_RequiredLayers.size());
        l_InstanceCreateInfo.ppEnabledLayerNames = m_RequiredLayers.data();

#ifdef _DEBUG
        const bool l_DebugUtilsEnabled = std::find(m_RequiredExtensions.begin(), m_RequiredExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != m_RequiredExtensions.end();
        const bool l_ValidationFeaturesEnabled =
            std::find(m_RequiredExtensions.begin(), m_RequiredExtensions.end(), VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME) != m_RequiredExtensions.end();

        const void* l_InstanceCreatePNext = nullptr;
        VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo{};
        VkValidationFeaturesEXT l_ValidationFeatures{};
        VkValidationFeatureEnableEXT l_EnabledValidationFeatures[] =
        {
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
        };

        if (l_ValidationFeaturesEnabled)
        {
            l_ValidationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            l_ValidationFeatures.enabledValidationFeatureCount = static_cast<uint32_t>(std::size(l_EnabledValidationFeatures));
            l_ValidationFeatures.pEnabledValidationFeatures = l_EnabledValidationFeatures;
            l_ValidationFeatures.pNext = l_InstanceCreatePNext;
            l_InstanceCreatePNext = &l_ValidationFeatures;
        }

        if (l_DebugUtilsEnabled)
        {
            l_DebugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            l_DebugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            l_DebugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            l_DebugCreateInfo.pfnUserCallback = Utilities::VulkanUtilities::VKDebugCallback;
            l_DebugCreateInfo.pNext = l_InstanceCreatePNext;
            l_InstanceCreatePNext = &l_DebugCreateInfo;
        }

        l_InstanceCreateInfo.pNext = l_InstanceCreatePNext;
#endif

        TR_CORE_TRACE("Creating instance with {} extensions and {} layers", l_InstanceCreateInfo.enabledExtensionCount, l_InstanceCreateInfo.enabledLayerCount);

        Utilities::VulkanUtilities::VKCheck(vkCreateInstance(&l_InstanceCreateInfo, m_Allocator, &m_Instance), "Failed vkCreateInstance");

        TR_CORE_TRACE("Vulkan Instance Created");
    }

    void VulkanContext::CreateSurface(const NativeWindowHandle& nativeWindowHandle)
    {
        TR_CORE_TRACE("Creating Window Surface");

        if (m_Instance == VK_NULL_HANDLE)
        {
            TR_CORE_CRITICAL("CreateSurface called without a valid VkInstance");

            std::abort();
        }

        m_Surface = CreateVulkanSurface(m_Instance, m_Allocator, nativeWindowHandle);

        TR_CORE_TRACE("Window Surface Created");
    }

    void VulkanContext::DestroySurface()
    {
        TR_CORE_TRACE("Destroying Window Surface");

        if (m_Surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_Instance, m_Surface, m_Allocator);
            m_Surface = VK_NULL_HANDLE;
        }

        TR_CORE_TRACE("Window Surface Destroyed");
    }

    void VulkanContext::DestroyInstance()
    {
        TR_CORE_TRACE("Destroying Vulkan Instance");

        if (m_Instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_Instance, m_Allocator);
            m_Instance = VK_NULL_HANDLE;
        }

        m_RequiredExtensions.clear();
        m_RequiredLayers.clear();

        TR_CORE_TRACE("Vulkan Instance Destroyed");
    }

    //-------------------------------------------------------------------------------------------------------------------------------------------------------------//

    std::vector<const char*> VulkanContext::GetRequiredExtensions() const
    {
        std::vector<const char*> l_Extensions{};

        l_Extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef _WIN32
        l_Extensions.push_back("VK_KHR_win32_surface");
#endif

#ifdef _DEBUG
        if (IsInstanceExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            l_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        else
        {
            TR_CORE_WARN("VK_EXT_debug_utils is not available. Validation output will be limited.");
        }

        if (IsInstanceExtensionSupported(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME))
        {
            l_Extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
        }
        else
        {
            TR_CORE_WARN("VK_EXT_validation_features is not available. Synchronization validation cannot be enabled.");
        }
#endif

        // Hard fail if a required extension is missing.
        for (const char* it_Extension : l_Extensions)
        {
            if (!IsInstanceExtensionSupported(it_Extension))
            {
                TR_CORE_CRITICAL("Required instance extension missing: {}", it_Extension);

                std::abort();
            }

#ifdef _DEBUG
            TR_CORE_TRACE("Required extension: {}", it_Extension);
#endif
        }

        return l_Extensions;
    }

    std::vector<const char*> VulkanContext::GetRequiredLayers() const
    {
        std::vector<const char*> l_Layers{};

#ifdef _DEBUG
        const char* l_ValidationLayer = "VK_LAYER_KHRONOS_validation";

        if (IsInstanceLayerSupported(l_ValidationLayer))
        {
            l_Layers.push_back(l_ValidationLayer);
            TR_CORE_TRACE("Validation layer enabled: {}", l_ValidationLayer);
        }
        else
        {
            TR_CORE_WARN("Requested validation layer not available: {}", l_ValidationLayer);
        }
#endif

        return l_Layers;
    }

    void VulkanContext::SetupDebugMessenger()
    {
#ifdef _DEBUG
        TR_CORE_TRACE("Setting Up Debug Messenger");

        const bool l_DebugUtilsEnabled = std::find(m_RequiredExtensions.begin(), m_RequiredExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != m_RequiredExtensions.end();
        if (!l_DebugUtilsEnabled)
        {
            TR_CORE_WARN("Skipping debug messenger setup (VK_EXT_debug_utils not enabled)");

            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT l_MessengerCreateInfo{};
        l_MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        l_MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        l_MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        l_MessengerCreateInfo.pfnUserCallback = Utilities::VulkanUtilities::VKDebugCallback;

        PFN_vkCreateDebugUtilsMessengerEXT l_CreateFunction = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
        if (!l_CreateFunction)
        {
            TR_CORE_WARN("vkCreateDebugUtilsMessengerEXT not found (debug messenger disabled)");

            return;
        }

        Utilities::VulkanUtilities::VKCheck(l_CreateFunction(m_Instance, &l_MessengerCreateInfo, m_Allocator, &m_DebugMessenger), "Failed vkCreateDebugUtilsMessengerEXT");

        TR_CORE_TRACE("Debug Messenger Setup Complete");
#endif
    }

    void VulkanContext::DestroyDebugMessenger()
    {
#ifdef _DEBUG
        TR_CORE_TRACE("Destroying Debug Messenger");

        if (m_DebugMessenger == VK_NULL_HANDLE)
        {
            TR_CORE_WARN("No Debug Messenger Found");

            return;
        }

        auto a_DestroyFunction = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (a_DestroyFunction)
        {
            a_DestroyFunction(m_Instance, m_DebugMessenger, m_Allocator);
            m_DebugMessenger = VK_NULL_HANDLE;
        }

        TR_CORE_TRACE("Debug Messenger Destroyed");
#endif
    }

    bool VulkanContext::IsInstanceExtensionSupported(const char* extensionName) const
    {
        uint32_t l_Count = 0;
        Utilities::VulkanUtilities::VKCheck(vkEnumerateInstanceExtensionProperties(nullptr, &l_Count, nullptr), "Failed vkEnumerateInstanceExtensionProperties");

        std::vector<VkExtensionProperties> l_Extensions(l_Count);
        Utilities::VulkanUtilities::VKCheck(vkEnumerateInstanceExtensionProperties(nullptr, &l_Count, l_Extensions.data()), "Failed vkEnumerateInstanceExtensionProperties");

        for (const auto& it_Extension : l_Extensions)
        {
            if (strcmp(it_Extension.extensionName, extensionName) == 0)
            {
                return true;
            }
        }

        return false;
    }

    bool VulkanContext::IsInstanceLayerSupported(const char* layerName) const
    {
        uint32_t l_Count = 0;
        Utilities::VulkanUtilities::VKCheck(vkEnumerateInstanceLayerProperties(&l_Count, nullptr), "Failed vkEnumerateInstanceLayerProperties");

        std::vector<VkLayerProperties> l_Layers(l_Count);
        Utilities::VulkanUtilities::VKCheck(vkEnumerateInstanceLayerProperties(&l_Count, l_Layers.data()), "Failed vkEnumerateInstanceLayerProperties");

        for (const auto& it_Layer : l_Layers)
        {
            if (strcmp(it_Layer.layerName, layerName) == 0)
            {
                return true;
            }
        }

        return false;
    }
}