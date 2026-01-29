#pragma once

#include <vector>

#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace Engine
{
    class VulkanContext
    {
    public:
        void Initialize(GLFWwindow* nativeWindow);
        void Shutdown();

        void CreateInstance();
        void SetupDebugMessenger();
        void CreateSurface();

        VkInstance GetInstance() const { return m_Instance; }
        VkSurfaceKHR GetSurface() const { return m_Surface; }
        VkDebugUtilsMessengerEXT GetDebugMessenger() const { return m_DebugMessenger; }
        GLFWwindow* GetNativeWindow() const { return m_NativeWindow; }
        bool UseValidationLayers() const { return m_UseValidationLayers; }

        static const std::vector<const char*>& GetValidationLayers();

    private:
        GLFWwindow* m_NativeWindow = nullptr;

        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

        bool m_UseValidationLayers = false;
    };
}