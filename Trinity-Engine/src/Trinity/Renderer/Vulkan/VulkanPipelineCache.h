#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace Trinity
{
    class VulkanPipelineCache
    {
    public:
        VulkanPipelineCache() = default;
        ~VulkanPipelineCache() = default;

        void Initialize(VkDevice device, const VkPhysicalDeviceProperties& properties, const std::string& path);
        void Shutdown();

        VkPipelineCache GetCache() const { return m_Cache; }
        const std::string& GetPath() const { return m_Path; }

    private:
        bool IsHeaderCompatible(const void* data, size_t size, const VkPhysicalDeviceProperties& properties) const;
        void SaveToDisk() const;

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkPipelineCache m_Cache = VK_NULL_HANDLE;
        std::string m_Path;
    };
}