#include "Engine/Renderer/Vulkan/VulkanContext.h"

#include "Engine/Platform/Window.h"
#include "Engine/Utilities/Utilities.h"

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <string>
#include <cstring>

namespace Engine
{
    void VulkanContext::Initialize(Window& window)
    {
        m_Window = &window;
        m_GLFWWindow = (GLFWwindow*)window.GetNativeWindow();

        CreateInstance();
        SetupDebugMessenger();
        CreateWindowSurface();

        TR_CORE_INFO("VulkanContext initialized");
    }

    void VulkanContext::Shutdown()
    {
        if (m_Surface && m_Instance)
        {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }

        if (m_EnableValidationLayers && m_DebugMessenger && m_Instance)
        {
            DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
            m_DebugMessenger = VK_NULL_HANDLE;
        }

        if (m_Instance)
        {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
        }

        m_GLFWWindow = nullptr;
        m_Window = nullptr;
    }

    void VulkanContext::CreateInstance()
    {
        if (m_EnableValidationLayers && !CheckValidationLayerSupport())
        {
            TR_CORE_CRITICAL("Validation layers requested, but not available");

            throw std::runtime_error("Validation layers not available");
        }

        VkApplicationInfo l_ApplicationInfo{};
        l_ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        l_ApplicationInfo.pApplicationName = "Engine";
        l_ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        l_ApplicationInfo.pEngineName = "Engine";
        l_ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        l_ApplicationInfo.apiVersion = VK_API_VERSION_1_4;

        auto a_Extensions = GetRequiredExtensions();

        VkInstanceCreateInfo l_InstanceCreateInfo{};
        l_InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        l_InstanceCreateInfo.pApplicationInfo = &l_ApplicationInfo;
        l_InstanceCreateInfo.enabledExtensionCount = (uint32_t)a_Extensions.size();
        l_InstanceCreateInfo.ppEnabledExtensionNames = a_Extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo{};
        if (m_EnableValidationLayers)
        {
            l_InstanceCreateInfo.enabledLayerCount = (uint32_t)m_ValidationLayers.size();
            l_InstanceCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();

            l_DebugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            l_DebugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            l_DebugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            l_DebugCreateInfo.pfnUserCallback = DebugCallback;

            l_InstanceCreateInfo.pNext = &l_DebugCreateInfo;
        }
        else
        {
            l_InstanceCreateInfo.enabledLayerCount = 0;
            l_InstanceCreateInfo.pNext = nullptr;
        }

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateInstance(&l_InstanceCreateInfo, nullptr, &m_Instance), "vkCreateInstance");
    }

    void VulkanContext::SetupDebugMessenger()
    {
        if (!m_EnableValidationLayers)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo{};
        l_DebugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        l_DebugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        l_DebugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        l_DebugCreateInfo.pfnUserCallback = DebugCallback;

        Utilities::VulkanUtilities::VKCheckStrict(CreateDebugUtilsMessengerEXT(m_Instance, &l_DebugCreateInfo, nullptr, &m_DebugMessenger), "CreateDebugUtilsMessengerEXT");
    }

    void VulkanContext::CreateWindowSurface()
    {
        Utilities::VulkanUtilities::VKCheckStrict(glfwCreateWindowSurface(m_Instance, m_GLFWWindow, nullptr, &m_Surface), "glfwCreateWindowSurface");
    }

    bool VulkanContext::CheckValidationLayerSupport() const
    {
        uint32_t l_LayerCount = 0;
        vkEnumerateInstanceLayerProperties(&l_LayerCount, nullptr);

        std::vector<VkLayerProperties> l_Layers(l_LayerCount);
        vkEnumerateInstanceLayerProperties(&l_LayerCount, l_Layers.data());

        for (const char* it_Name : m_ValidationLayers)
        {
            bool l_Found = false;
            for (const auto& it_Layer : l_Layers)
            {
                if (std::strcmp(it_Name, it_Layer.layerName) == 0)
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

    std::vector<const char*> VulkanContext::GetRequiredExtensions() const
    {
        uint32_t l_GLFWCount = 0;
        const char** l_GLFWExtensions = glfwGetRequiredInstanceExtensions(&l_GLFWCount);

        std::vector<const char*> l_Extensions(l_GLFWExtensions, l_GLFWExtensions + l_GLFWCount);

        if (m_EnableValidationLayers)
        {
            l_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return l_Extensions;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
    {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            TR_CORE_ERROR("Vulkan validation: {}", pCallbackData->pMessage);
        }
        else
        {
            TR_CORE_WARN("Vulkan validation: {}", pCallbackData->pMessage);
        }

        return VK_FALSE;
    }

    VkResult VulkanContext::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
    {
        auto a_Function = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (a_Function != nullptr)
        {
            return a_Function(instance, pCreateInfo, pAllocator, pMessenger);
        }

        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void VulkanContext::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
    {
        auto a_Function = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (a_Function != nullptr)
        {
            a_Function(instance, messenger, pAllocator);
        }
    }
}