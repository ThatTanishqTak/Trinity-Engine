#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine
{
    class VulkanDevice;

    class VulkanDescriptors
    {
    public:
        VulkanDescriptors() = default;
        ~VulkanDescriptors() = default;

        VulkanDescriptors(const VulkanDescriptors&) = delete;
        VulkanDescriptors& operator=(const VulkanDescriptors&) = delete;
        VulkanDescriptors(VulkanDescriptors&&) = delete;
        VulkanDescriptors& operator=(VulkanDescriptors&&) = delete;

        // Owns the descriptor set layout and pool. These are device-level objects that
        // can survive swapchain recreation, but must be recreated if the device or layout changes.
        void Initialize(VulkanDevice& device, uint32_t framesInFlight);
        void Shutdown(VulkanDevice& device);
        void Shutdown(VulkanDevice& device, const std::function<void(std::function<void()>&&)>& submitResourceFree);

        bool IsValid() const { return m_Layout != VK_NULL_HANDLE && m_Pool != VK_NULL_HANDLE && !m_DescriptorSets.empty(); }

        VkDescriptorSetLayout GetLayout() const { return m_Layout; }
        VkDescriptorSet GetDescriptorSet(uint32_t frameIndex) const;

    private:
        VulkanDevice* m_Device = nullptr;
        VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
        VkDescriptorPool m_Pool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_DescriptorSets;
    };
}