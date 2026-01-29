#pragma once

#include <optional>

#include "Engine/Renderer/Vulkan/VulkanContext.h"

struct GLFWwindow;

namespace Engine
{
    class VulkanDevice
    {
    public:
        struct QueueFamilyIndices
        {
            std::optional<uint32_t> GraphicsFamily;
            std::optional<uint32_t> PresentFamily;

            bool IsComplete() const { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
        };

        void Initialize(GLFWwindow* nativeWindow);
        void Shutdown();

        VkInstance GetInstance() const { return m_Context.GetInstance(); }
        VkSurfaceKHR GetSurface() const { return m_Context.GetSurface(); }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkDevice GetDevice() const { return m_Device; }
        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }
        QueueFamilyIndices GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }

    private:
        void PickPhysicalDevice();
        void CreateLogicalDevice();

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
        uint32_t RateDeviceSuitability(VkPhysicalDevice device) const;

    private:
        GLFWwindow* m_NativeWindow = nullptr;
        QueueFamilyIndices m_QueueFamilyIndices;

        VulkanContext m_Context;

        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;

        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;
    };
}