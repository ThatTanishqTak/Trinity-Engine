#include "Engine/Renderer/Vulkan/VulkanDevice.h"

#include "Engine/Renderer/Vulkan/VulkanSwapchain.h"
#include "Engine/Utilities/Utilities.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <set>
#include <string>
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

    const std::vector<const char*> s_DeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
    void VulkanDevice::Initialize(GLFWwindow* nativeWindow)
    {
        m_NativeWindow = nativeWindow;

        if (!m_NativeWindow)
        {
            TR_CORE_CRITICAL("VulkanDevice: Native window handle is null (expected GLFWwindow*).");

            std::abort();
        }

        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
    }

    void VulkanDevice::Shutdown()
    {
        if (m_Device)
        {
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        if (s_EnableValidationLayers && m_DebugMessenger)
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

        m_PhysicalDevice = VK_NULL_HANDLE;
        m_GraphicsQueue = VK_NULL_HANDLE;
        m_PresentQueue = VK_NULL_HANDLE;
        m_QueueFamilyIndices = {};
        m_NativeWindow = nullptr;
    }

    void VulkanDevice::CreateInstance()
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

        bool l_UseValidation = s_EnableValidationLayers && CheckValidationLayerSupport();
        if (l_UseValidation)
        {
            l_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        VkInstanceCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        l_Info.pApplicationInfo = &l_App;
        l_Info.enabledExtensionCount = static_cast<uint32_t>(l_Extensions.size());
        l_Info.ppEnabledExtensionNames = l_Extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT l_DebugInfo{};
        if (l_UseValidation)
        {
            l_Info.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
            l_Info.ppEnabledLayerNames = s_ValidationLayers.data();

            l_DebugInfo = MakeDebugMessengerCreateInfo();
            l_Info.pNext = &l_DebugInfo;
        }

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateInstance(&l_Info, nullptr, &m_Instance), "vkCreateInstance");
    }

    void VulkanDevice::SetupDebugMessenger()
    {
        if (!s_EnableValidationLayers)
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

    void VulkanDevice::CreateSurface()
    {
        Engine::Utilities::VulkanUtilities::VKCheckStrict(glfwCreateWindowSurface(m_Instance, m_NativeWindow, nullptr, &m_Surface), "glfwCreateWindowSurface");
    }

    VulkanDevice::QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device) const
    {
        QueueFamilyIndices l_Indices;

        uint32_t l_Count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_Count, nullptr);

        std::vector<VkQueueFamilyProperties> l_Families(l_Count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &l_Count, l_Families.data());

        uint32_t l_Index = 0;
        for (const auto& it_Family : l_Families)
        {
            if (it_Family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                l_Indices.GraphicsFamily = l_Index;
            }

            VkBool32 l_PresentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, l_Index, m_Surface, &l_PresentSupport);
            if (l_PresentSupport)
            {
                l_Indices.PresentFamily = l_Index;
            }

            if (l_Indices.IsComplete())
            {
                break;
            }

            ++l_Index;
        }

        return l_Indices;
    }

    bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
    {
        uint32_t l_Count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &l_Count, nullptr);

        std::vector<VkExtensionProperties> l_Available(l_Count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &l_Count, l_Available.data());

        std::set<std::string> l_Required(s_DeviceExtensions.begin(), s_DeviceExtensions.end());
        for (const auto& it_Ext : l_Available)
        {
            l_Required.erase(it_Ext.extensionName);
        }

        return l_Required.empty();
    }

    uint32_t VulkanDevice::RateDeviceSuitability(VkPhysicalDevice device) const
    {
        QueueFamilyIndices l_Indices = FindQueueFamilies(device);
        if (!l_Indices.IsComplete())
        {
            return 0;
        }

        if (!CheckDeviceExtensionSupport(device))
        {
            return 0;
        }

        const auto l_Support = Engine::VulkanSwapchain::QuerySupport(device, m_Surface);
        if (l_Support.Formats.empty() || l_Support.PresentModes.empty())
        {
            return 0;
        }

        VkPhysicalDeviceProperties l_Props{};
        vkGetPhysicalDeviceProperties(device, &l_Props);

        VkPhysicalDeviceMemoryProperties l_MemProps{};
        vkGetPhysicalDeviceMemoryProperties(device, &l_MemProps);

        VkDeviceSize l_DeviceLocalBytes = 0;
        for (uint32_t it_Heap = 0; it_Heap < l_MemProps.memoryHeapCount; ++it_Heap)
        {
            const VkMemoryHeap& l_Heap = l_MemProps.memoryHeaps[it_Heap];
            if (l_Heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                l_DeviceLocalBytes = std::max(l_DeviceLocalBytes, l_Heap.size);
            }
        }

        uint32_t l_Score = 0;

        switch (l_Props.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   l_Score += 100000; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: l_Score += 10000;  break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    l_Score += 1000;   break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            l_Score += 100;    break;
            default: break;
        }

        l_Score += static_cast<uint32_t>(l_DeviceLocalBytes / (1024ull * 1024ull * 1024ull)) * 1000u;
        l_Score += l_Props.limits.maxImageDimension2D;

        return l_Score;
    }

    void VulkanDevice::PickPhysicalDevice()
    {
        uint32_t l_Count = 0;
        vkEnumeratePhysicalDevices(m_Instance, &l_Count, nullptr);

        if (l_Count == 0)
        {
            TR_CORE_CRITICAL("No Vulkan-compatible GPUs found.");

            std::abort();
        }

        std::vector<VkPhysicalDevice> l_Devices(l_Count);
        vkEnumeratePhysicalDevices(m_Instance, &l_Count, l_Devices.data());

        VkPhysicalDevice l_BestDevice = VK_NULL_HANDLE;
        uint32_t l_BestScore = 0;

        for (VkPhysicalDevice it_Device : l_Devices)
        {
            uint32_t l_Score = RateDeviceSuitability(it_Device);
            if (l_Score > l_BestScore)
            {
                l_BestScore = l_Score;
                l_BestDevice = it_Device;
            }
        }

        m_PhysicalDevice = l_BestDevice;

        if (m_PhysicalDevice == VK_NULL_HANDLE || l_BestScore == 0)
        {
            TR_CORE_CRITICAL("Failed to find a suitable GPU.");

            std::abort();
        }

        VkPhysicalDeviceProperties l_SelectedProps{};
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &l_SelectedProps);
        TR_CORE_TRACE("Vulkan selected GPU: {}", l_SelectedProps.deviceName);
    }

    void VulkanDevice::CreateLogicalDevice()
    {
        m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> l_QueueInfos;
        std::set<uint32_t> l_UniqueFamilies =
        {
            m_QueueFamilyIndices.GraphicsFamily.value(),
            m_QueueFamilyIndices.PresentFamily.value()
        };

        float l_Priority = 1.0f;
        for (uint32_t it_Family : l_UniqueFamilies)
        {
            VkDeviceQueueCreateInfo l_Q{};
            l_Q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            l_Q.queueFamilyIndex = it_Family;
            l_Q.queueCount = 1;
            l_Q.pQueuePriorities = &l_Priority;
            l_QueueInfos.push_back(l_Q);
        }

        VkPhysicalDeviceFeatures l_Features{};

        VkDeviceCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        l_Info.queueCreateInfoCount = static_cast<uint32_t>(l_QueueInfos.size());
        l_Info.pQueueCreateInfos = l_QueueInfos.data();
        l_Info.pEnabledFeatures = &l_Features;
        l_Info.enabledExtensionCount = static_cast<uint32_t>(s_DeviceExtensions.size());
        l_Info.ppEnabledExtensionNames = s_DeviceExtensions.data();

        if (s_EnableValidationLayers)
        {
            l_Info.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
            l_Info.ppEnabledLayerNames = s_ValidationLayers.data();
        }

        Engine::Utilities::VulkanUtilities::VKCheckStrict(vkCreateDevice(m_PhysicalDevice, &l_Info, nullptr, &m_Device), "vkCreateDevice");

        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.PresentFamily.value(), 0, &m_PresentQueue);
    }
}