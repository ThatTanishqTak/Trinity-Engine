#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan.h>

namespace Trinity
{
    class VulkanInstance
    {
    public:
        VulkanInstance() = default;
        ~VulkanInstance();

        VulkanInstance(const VulkanInstance&) = delete;
        VulkanInstance& operator=(const VulkanInstance&) = delete;

        bool Initialize(const std::string& applicationName, bool enableValidation);
        void Shutdown();

        VkInstance GetHandle() const { return m_Instance; }
        bool IsValidationEnabled() const { return m_ValidationEnabled; }

    private:
        bool CreateInstance(const std::string& applicationName);
        bool SetupDebugMessenger();

        std::vector<const char*> GetRequiredExtensions() const;
        std::vector<const char*> GetRequiredLayers() const;

        bool CheckLayerSupport(const std::vector<const char*>& requestedLayers) const;

    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        bool m_ValidationEnabled = false;
    };
}