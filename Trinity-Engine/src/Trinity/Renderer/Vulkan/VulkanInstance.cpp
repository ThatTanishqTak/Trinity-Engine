#include "Trinity/Renderer/Vulkan/VulkanInstance.h"

#include "Trinity/Renderer/Vulkan/VulkanDebug.h"
#include "Trinity/Renderer/Vulkan/VulkanUtilities.h"
#include "Trinity/Platform/Window/Window.h"

#include <SDL3/SDL_vulkan.h>

#include <cstdlib>
#include <cstring>
#include <vector>

namespace Trinity
{
    static const std::vector<const char*> s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

    void VulkanInstance::Initialize(Window& window, bool enableValidation)
    {
        m_ValidationEnabled = enableValidation;

        CreateInstance(enableValidation);
        CreateSurface(window);
    }

    void VulkanInstance::Shutdown()
    {
        if (m_Surface != VK_NULL_HANDLE)
        {
            TR_CORE_TRACE("Destroying Vulkan Surface");

            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;

            TR_CORE_TRACE("Vulkan Surface Destroyed");
        }

        if (m_Instance != VK_NULL_HANDLE)
        {
            TR_CORE_TRACE("Destroying Vulkan Instnace");

            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;

            TR_CORE_TRACE("Vulkan Instnace Destroyed");
        }
    }

    void VulkanInstance::CreateInstance(bool enableValidation)
    {
        TR_CORE_TRACE("Creating Vulkan Instance");

        if (enableValidation && !CheckValidationLayerSupport())
        {
            m_ValidationEnabled = false;
            enableValidation = false;
        }

        uint32_t l_AvailableExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &l_AvailableExtensionCount, nullptr);
        std::vector<VkExtensionProperties> l_AvailableExtensions(l_AvailableExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &l_AvailableExtensionCount, l_AvailableExtensions.data());

        auto l_HasInstanceExtension = [&l_AvailableExtensions](const char* name)
        {
            for (const auto& it_Extension : l_AvailableExtensions)
            {
                if (strcmp(it_Extension.extensionName, name) == 0)
                {
                    return true;
                }
            }
            return false;
        };

        VkApplicationInfo l_ApplicationInfo{};
        l_ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        l_ApplicationInfo.pApplicationName = "Trinity Engine";
        l_ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        l_ApplicationInfo.pEngineName = "Trinity";
        l_ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        l_ApplicationInfo.apiVersion = VK_API_VERSION_1_3;

        uint32_t l_SDLExtensionCount = 0;
        const char* const* l_SDLExtensions = SDL_Vulkan_GetInstanceExtensions(&l_SDLExtensionCount);

        std::vector<const char*> l_Extensions(l_SDLExtensions, l_SDLExtensions + l_SDLExtensionCount);

        if (enableValidation)
        {
            l_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

#ifdef __APPLE__
        l_Extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

        if (l_HasInstanceExtension(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME))
        {
            l_Extensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
            m_HasSwapchainColorspace = true;
            TR_CORE_TRACE("Optional instance extension enabled: {}", VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
        }
        else
        {
            TR_CORE_TRACE("Optional instance extension not available: {}", VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
        }

        VkInstanceCreateInfo l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        l_CreateInfo.pApplicationInfo = &l_ApplicationInfo;
        l_CreateInfo.enabledExtensionCount = static_cast<uint32_t>(l_Extensions.size());
        l_CreateInfo.ppEnabledExtensionNames = l_Extensions.data();

#ifdef __APPLE__
        l_CreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo{};
        if (enableValidation)
        {
            l_CreateInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
            l_CreateInfo.ppEnabledLayerNames = s_ValidationLayers.data();

            VulkanDebug::PopulateCreateInfo(l_DebugCreateInfo);
            l_CreateInfo.pNext = &l_DebugCreateInfo;
        }
        else
        {
            l_CreateInfo.enabledLayerCount = 0;
            l_CreateInfo.pNext = nullptr;
        }

        VulkanUtilities::VKCheck(vkCreateInstance(&l_CreateInfo, nullptr, &m_Instance), "Failed vkCreateInstance");

        TR_CORE_TRACE("Vulkan Instance Created");
    }

    void VulkanInstance::CreateSurface(Window& window)
    {
        TR_CORE_TRACE("Creating Vulkan Surface");

        NativeWindowHandle l_Handle = window.GetNativeHandle();
        if (!SDL_Vulkan_CreateSurface(l_Handle.Window, m_Instance, nullptr, &m_Surface))
        {
            TR_CORE_CRITICAL("Failed to create vulkan surface");
            std::abort();
        }

        TR_CORE_TRACE("Vulkan Surface Created");
    }

    bool VulkanInstance::CheckValidationLayerSupport() const
    {
        uint32_t l_LayerCount = 0;
        vkEnumerateInstanceLayerProperties(&l_LayerCount, nullptr);

        std::vector<VkLayerProperties> l_AvailableLayers(l_LayerCount);
        vkEnumerateInstanceLayerProperties(&l_LayerCount, l_AvailableLayers.data());

        for (const char* it_LayerName : s_ValidationLayers)
        {
            bool l_Found = false;
            for (const auto& it_LayerProperties : l_AvailableLayers)
            {
                if (strcmp(it_LayerName, it_LayerProperties.layerName) == 0)
                {
                    l_Found = true;
                    TR_CORE_TRACE("Validation Layer Found: {}", it_LayerProperties.layerName);

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