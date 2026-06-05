#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace Trinity
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> Graphics;
        std::optional<uint32_t> Present;

        bool IsComplete() const
        {
            return Graphics.has_value() && Present.has_value();
        }
    };

    struct VulkanFeatures
    {
        bool DynamicRendering = true;
        bool Synchronization2 = true;
        bool TimelineSemaphores = true;
        bool ShaderDrawParameters = true;
    };

    class VulkanPhysicalDevice
    {
    public:
        VulkanPhysicalDevice() = default;

        bool Select(VkInstance instance, VkSurfaceKHR surface, const VulkanFeatures& required);

        VkPhysicalDevice GetHandle() const { return m_Device; }
        const QueueFamilyIndices& GetQueueFamilies() const { return m_QueueFamilies; }
        const std::string& GetName() const { return m_Name; }
        const VkPhysicalDeviceProperties& GetProperties() const { return m_Properties; }
        const std::vector<const char*>& GetRequiredExtensions() const { return m_RequiredExtensions; }

    private:
        bool IsSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const VulkanFeatures& required) const;
        
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;
        
        bool CheckExtensionSupport(VkPhysicalDevice device) const;
        bool CheckFeatureSupport(VkPhysicalDevice device, const VulkanFeatures& required) const;
        
        uint32_t ScoreDevice(VkPhysicalDevice device) const;

    private:
        VkPhysicalDevice m_Device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_Properties{};
        
        QueueFamilyIndices m_QueueFamilies;

        std::string m_Name;
        std::vector<const char*> m_RequiredExtensions;
    };
}