#pragma once

#include <vulkan/vulkan.h>

#include <vector>

struct GLFWwindow;

namespace Engine
{
    class Window;

    class VulkanContext
    {
    public:
        VulkanContext() = default;
        ~VulkanContext() = default;

        VulkanContext(const VulkanContext&) = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;
        VulkanContext(VulkanContext&&) = delete;
        VulkanContext& operator=(VulkanContext&&) = delete;

        void Initialize(Window& window);
        void Shutdown();

        VkInstance GetInstance() const { return m_Instance; }
        VkSurfaceKHR GetSurface() const { return m_Surface; }
        bool ValidationEnabled() const { return m_EnableValidationLayers; }

    private:
        void CreateInstance();
        void SetupDebugMessenger();
        void CreateWindowSurface();

        bool CheckValidationLayerSupport() const;
        std::vector<const char*> GetRequiredExtensions() const;

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

        static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
            const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger);

        static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator);

    private:
        Window* m_Window = nullptr;
        GLFWwindow* m_GLFWWindow = nullptr;

        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

#ifdef TR_ENABLE_VK_VALIDATION
        const bool m_EnableValidationLayers = true;
#else
        const bool m_EnableValidationLayers = false;
#endif

        const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
    };
}